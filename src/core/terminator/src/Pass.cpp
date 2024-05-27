#include <set>

#include "arcana/gino/core/DOALL.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/IR/Instructions.h"

#include "Analysis.hpp"
#include "Pass.hpp"

using namespace std;
using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

TerminatorPass::TerminatorPass() : ModulePass{ID}, prefix("Terminator: ") {}

bool TerminatorPass::doInitialization(Module &M) { return false; }

void TerminatorPass::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<ScalarEvolutionWrapperPass>();
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<Noelle>();

  return;
}

bool TerminatorPass::runOnModule(Module &M) {
  auto &noelle = getAnalysis<Noelle>();
  auto MM = noelle.getMetadataManager();

  // Analyzing only LoopStructures selected by the planner
  set<LoopStructure *> LSs;
  for (auto *LS : *noelle.getLoopStructures()) {
    if (MM->doesHaveMetadata(LS, "noelle.parallelizer.looporder")) {
      LSs.insert(LS);
    }
  }

  TerminatorAnalysis TA(noelle);
  TA.run(LSs);
  noelle.addAnalysis(&TA);

  for (auto *LS : LSs) {
    auto loopID = LS->getID().value();
    errs() << this->prefix << "loopID = " << loopID << "\n";
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
