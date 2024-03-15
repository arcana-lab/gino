#include <string>

#include "TerminatorAnalysis.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/IR/Instructions.h"

using namespace std;
using namespace llvm;

namespace arcana::gino {

static int terminationCounter = 0;

void terminateLCDs(noelle::LoopStructure *LS, TerminatorAnalysis &TA) {
  auto clauses = TA.getClausesOf(LS);

  errs() << "Terminator: Termination for loop " << LS->getID().value() << " ("
         << clauses.size() << " clauses)\n";

  for (auto &clause : clauses) {
    auto *F = clause->getFunction();
    auto branchInst = LS->getPreHeader()->getTerminator();
    auto TCName = "TCValue." + to_string(terminationCounter);
    auto TCValue =
        CallInst::Create(F, clause->getCallArguments(), TCName, branchInst);
    StoreInst *SI = new StoreInst(TCValue, clause->getVariable(), branchInst);
    terminationCounter++;
  }
}

} // namespace arcana::gino
