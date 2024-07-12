#include <set>

#include "arcana/gino/core/DOALL.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"

#include "Analysis.hpp"
#include "Pass.hpp"
#include "arcana/gino/core/LoopBlocker.hpp"

using namespace std;
using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

static cl::opt<int> NumBreaks("terminator-breaks", cl::ZeroOrMore, cl::init(8),
                              cl::Hidden,
                              cl::desc("Number of times we break a LCD"));

TerminatorPass::TerminatorPass() : ModulePass{ID}, prefix("Terminator: ") {}

bool TerminatorPass::doInitialization(Module &M) { return false; }

void TerminatorPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<Noelle>();
  AU.addRequired<HeuristicsPass>();

  return;
}

bool TerminatorPass::runOnModule(Module &M) {
  auto &noelle = getAnalysis<Noelle>();
  auto MM = noelle.getMetadataManager();

  // Phase 1
  // Analyzing only LoopStructures selected by the planner
  set<LoopStructure *> LSs;
  for (auto *LS : *noelle.getLoopStructures()) {
    if (MM->doesHaveMetadata(LS, "noelle.parallelizer.looporder")) {
      LSs.insert(LS);
    }
  }

  TerminatorAnalysis TA(noelle);
  TA.run(LSs);

  auto heuristics = getAnalysis<HeuristicsPass>().getHeuristics(noelle);

  auto getLoopDescription = [&MM](LoopStructure *LS) {
    auto loopID = LS->getID().value();
    auto loopOrder = MM->getMetadata(LS, "noelle.parallelizer.looporder");
    return "(id=" + to_string(loopID) + ", order=" + loopOrder + ")";
  };

  set<LoopStructure *> retryLSs;
  const DOALL doall(noelle);

  // Phase 2
  // Identifying non-DOALL loops from the loop with clauses
  auto optimizations = {LoopContentOptimization::MEMORY_CLONING_ID,
                        LoopContentOptimization::THREAD_SAFE_LIBRARY_ID};
  auto plannedLSs = TA.getLoopStructuresWithClauses();
  for (auto *LS : plannedLSs) {
    auto LC = noelle.getLoopContent(LS, optimizations);
    auto LD = getLoopDescription(LS);
    bool isDOALL = doall.canBeAppliedToLoop(LC, heuristics);

    errs() << this->prefix << "loop " << LD << " is " << (isDOALL ? "" : "not ")
           << "DOALL\n";

    if (isDOALL) {
      MM->addMetadata(LS, "gino.doall", "yes");
    } else {
      retryLSs.insert(LS);
    }
  }

  noelle.addAnalysis(&TA);
  errs() << this->prefix << "Added Termination engine to Noelle\n";

  // Phase 3
  // Exploiting Termination clauses on the non-DOALL loops
  set<LoopContent *> terminationTargetLCs;
  for (auto *LS : retryLSs) {
    auto LC = noelle.getLoopContent(LS, optimizations);
    auto LD = getLoopDescription(LS);
    bool isDOALL = doall.canBeAppliedToLoop(LC, heuristics);

    errs() << this->prefix << "loop " << LD << " can "
           << (isDOALL ? "" : "not ") << "be DOALL\n";

    if (isDOALL) {
      terminationTargetLCs.insert(LC);
    } else {
      // Despite the termination clauses, this loop is stil hopeless
      MM->addMetadata(LS, "gino.doall", "no");
    }
  }

  // Phase 4
  // Applying loop blocking transformation to the termination targets
  IRBuilder<> Builder(M.getContext());
  const int NumBlocks = NumBreaks + 1;

  for (auto *LC : terminationTargetLCs) {
    auto LS = LC->getLoopStructure();
    BasicBlock *NewHeader = blockLoop(LC, NumBlocks);
    auto LD = getLoopDescription(LS);
    if (NewHeader == nullptr) {
      errs() << this->prefix << "Failed to block loop " << LD << "\n";
    } else {
      errs() << this->prefix << "Blocked loop " << LD
             << ", blocks=" << NumBlocks << "\n";
    }

    auto &FirstPHI = *NewHeader->phis().begin();
    // The old preheader is still stored in LS.
    // At this point, this is the preheader of the new loop introduced
    // by the loop blocking transformation.
    auto PreHeader = LS->getPreHeader();
    auto Zero = Builder.getInt32(0);
    Builder.SetInsertPoint(PreHeader->getTerminator());

    // Applying termination clauses
    for (auto &clause : TA.getClausesOf(LS)) {
      auto ClausePtrTy = clause->getVariable()->getType();
      auto ClauseElemTy = ClausePtrTy->getPointerElementType();
      auto ArrayTy = ArrayType::get(ClauseElemTy, NumBlocks);
      auto name = "TCValues.loopid." + to_string(LS->getID().value());

      // Generating the necessary calls to the clause function
      auto OriginalVariableValue =
          Builder.CreateLoad(ClauseElemTy, clause->getVariable());
      auto TCValues = Builder.CreateAlloca(ArrayTy, nullptr, name);

      for (int i = 0; i < NumBlocks; i++) {
        auto GEP = Builder.CreateInBoundsGEP(ArrayTy, TCValues,
                                             {Zero, Builder.getInt32(i)});
        if (i == 0) {
          Builder.CreateStore(clause->getDefaultValue(), GEP);
        } else {
          auto TCValue = Builder.CreateCall(clause->getFunction(),
                                            clause->getCallArguments());
          Builder.CreateStore(TCValue, GEP);
        }
      }

      // Patching the clause variable with a value from TCValues
      Builder.SetInsertPoint(NewHeader->getTerminator());
      auto GEP =
          Builder.CreateInBoundsGEP(ArrayTy, TCValues, {Zero, &FirstPHI});
      auto LoadTCValue = Builder.CreateLoad(ClauseElemTy, GEP);
      Builder.SetInsertPoint(clause->getBegin());
      Builder.CreateStore(LoadTCValue, clause->getVariable());
    }

    // Moving the looporder metadata to the new outer loop
    auto LO = MM->getMetadata(LS, "noelle.parallelizer.looporder");
    MM->deleteMetadata(LS->getHeader()->getTerminator(),
                       "noelle.parallelizer.looporder");
    MM->addMetadata(NewHeader->getTerminator(), "noelle.parallelizer.looporder",
                    LO);

    // Marking the new loop as DOALL
    MM->addMetadata(NewHeader->getTerminator(), "gino.doall", "yes");
  }

  return false;
}

char TerminatorPass::ID = 0;
static RegisterPass<TerminatorPass>
    X("Terminator", "Enabler that uses Termination Clauses", false, false);

} // namespace arcana::gino
