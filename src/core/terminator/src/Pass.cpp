#include <set>

#include "arcana/gino/core/DOALL.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/IR/Instructions.h"

#include "Analysis.hpp"
#include "Pass.hpp"
#include "arcana/gino/core/LoopBlocker.hpp"

using namespace std;
using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

static cl::opt<int> NumBlocks("terminator-blocks", cl::ZeroOrMore, cl::init(8),
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

  auto optimizations = {LoopContentOptimization::MEMORY_CLONING_ID,
                        LoopContentOptimization::THREAD_SAFE_LIBRARY_ID};
  auto plannedLSs = TA.getLoopStructuresWithClauses();
  auto heuristics = getAnalysis<HeuristicsPass>().getHeuristics(noelle);

  auto getLoopDescription = [&MM](LoopStructure *LS) {
    auto loopID = LS->getID().value();
    auto loopOrder = MM->getMetadata(LS, "noelle.parallelizer.looporder");
    return "(id=" + to_string(loopID) + ", order=" + loopOrder + ")";
  };

  set<LoopStructure *> retryLSs;
  DOALL doall(noelle);

  // Phase 2
  // Identifying non-DOALL loops
  for (auto *LS : plannedLSs) {
    auto LC = noelle.getLoopContent(LS, optimizations);
    auto LD = getLoopDescription(LS);
    bool isDOALL = doall.canBeAppliedToLoop(LC, heuristics);

    errs() << this->prefix << "loop " << LD << " is " << (isDOALL ? "" : "not ")
           << "DOALL\n";

    if (!isDOALL) {
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

    errs() << this->prefix << "loop " << LD << " is " << (isDOALL ? "" : "not ")
           << "DOALL\n";

    if (isDOALL) {
      terminationTargetLCs.insert(LC);
    }
  }

  // Phase 4
  // Applying loop blocking transformation to the termination targets
  for (auto *LC : terminationTargetLCs) {
    auto LS = LC->getLoopStructure();
    auto header = blockLoop(LC, NumBlocks);
    auto LD = getLoopDescription(LS);
    if (header == nullptr) {
      errs() << this->prefix << "Failed to block loop " << LD << "\n";
    } else {
      errs() << this->prefix << "Blocked loop " << LD
             << ", blocks=" << NumBlocks << "\n";
    }

    // Applying termination clauses
    // TODO

    // Moving the looporder metadata to the new outer loop
    auto LO = MM->getMetadata(LS, "noelle.parallelizer.looporder");
    MM->deleteMetadata(LS->getHeader()->getTerminator(),
                       "noelle.parallelizer.looporder");
    MM->addMetadata(header->getTerminator(), "noelle.parallelizer.looporder",
                    LO);

    // Marking the new loop as DOALL
    MM->addMetadata(header->getTerminator(), "gino.doall", "");
  }

  // // From the old Terminator
  // for (auto &clause : clauses) {
  //   auto *F = clause->getFunction();
  //   auto branchInst = LS->getPreHeader()->getTerminator();
  //   auto TCName = "TCValue." + to_string(terminationCounter);
  //   auto TCValue =
  //       CallInst::Create(F, clause->getCallArguments(), TCName, branchInst);
  //   StoreInst *SI = new StoreInst(TCValue, clause->getVariable(),
  //   branchInst); terminationCounter++;
  // }

  // // Add metadata
  // auto MM = noelle.getMetadataManager();
  // MM->addMetadata(header->getTerminator(), "gino.doall", "");

  return false;
}

char TerminatorPass::ID = 0;
static RegisterPass<TerminatorPass>
    X("Terminator", "Enabler that uses Termination Clauses", false, false);

} // namespace arcana::gino
