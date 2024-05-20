#include <string>

#include "TerminatorAnalysis.hpp"
#include "arcana/gino/core/DOALL.hpp"
#include "arcana/gino/core/Winchester.hpp"
#include "noelle/core/InductionVariableSCC.hpp"
#include "noelle/core/Noelle.hpp"
#include "noelle/core/Task.hpp"
#include "llvm/IR/Instructions.h"

using namespace std;
using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

static int terminationCounter = 0;

// class LoopSliceTask : public Task {
// public:
//   LoopSliceTask(Module *module, FunctionType *loopSliceTaskSignature,
//                 std::string &loopSliceTaskName);
//
//   inline Value *getTaskMemoryPointerArg() {
//     return this->taskMemoryPointerArg;
//   };
//   inline Value *getTaskIndexArg() { return this->taskIndexArg; };
//   inline Value *getLSTContextPointerArg() {
//     return this->LSTContextPointerArg;
//   };
//   inline Value *getInvariantsPointerArg() {
//     return this->invariantsPointerArg;
//   };
//
// private:
//   Value *taskMemoryPointerArg;
//   Value *taskIndexArg;
//   Value *LSTContextPointerArg;
//   Value *invariantsPointerArg;
// };

// void Winchester::sliceLoop(Noelle &noelle, LoopContent *LC) {
//   auto LS = LC->getLoopStructure();
//   auto loopHeader = LS->getHeader();
//   auto loopPreHeader = LS->getPreHeader();
//   auto loopFunction = LS->getFunction();
//   auto loopEnvironment = LC->getEnvironment();
//   assert(loopEnvironment != nullptr);
//   auto ltm = LC->getLoopTransformationsManager();
//   auto maxCores = ltm->getMaximumNumberOfCores();
//
//   // Definie task signature
//   auto tm = this->n.getTypesManager();
//   auto funcArgTypes =
//       ArrayRef<Type *>({tm->getVoidPointerType(), tm->getIntegerType(64),
//                         tm->getIntegerType(64), tm->getIntegerType(64)});
//   auto taskSignature =
//       FunctionType::get(tm->getVoidType(), funcArgTypes, false);
//
//   // Generate an empty task for the parallel DOALL execution.
//   auto lsTask = new Task(taskSignature, *noelle.getProgram(), "lsTask");
//   this->fromTaskIDToUserID[lsTask->getID()] = 0;
//   this->addPredecessorAndSuccessorsBasicBlocksToTasks(LC, {lsTask});
//   this->numTaskInstances = maxCores;
//
//   auto sccManager = LC->getSCCManager();
//   auto variablesToBeReduced = [](uint32_t id, bool isLiveOut) { return false;
//   }; auto variablesToBeSkipped = [](uint32_t id, bool isLiveOut) { return
//   true; }; this->initializeEnvironmentBuilder(LC, variablesToBeReduced,
//                                      variablesToBeSkipped);
//   this->cloneSequentialLoop(LC, 0);
//
//   // Load live-in values at the entry point of the task
//   auto envUser = this->envBuilder->getUser(0);
//   assert(envUser != nullptr);
//   for (auto envID : loopEnvironment->getEnvIDsOfLiveInVars()) {
//     envUser->addLiveIn(envID);
//   }
//   for (auto envID : loopEnvironment->getEnvIDsOfLiveOutVars()) {
//     envUser->addLiveOut(envID);
//   }
//   this->generateCodeToLoadLiveInVariables(LC, 0);
//
//   // This must follow loading live-ins as this re-wiring
//   // overrides the live-in mapping to use locally cloned memory instructions
//   // that are live-in to the loop
//   if (ltm->isOptimizationEnabled(LoopContentOptimization::MEMORY_CLONING_ID))
//   {
//     this->cloneMemoryLocationsLocallyAndRewireLoop(LC, 0);
//   }
//   lsTask->adjustDataAndControlFlowToUseClones();
//
//   // Handle the reduction variables.
//   this->setReducableVariablesToBeginAtIdentityValue(LC, 0);
//
//   // Add the jump to start the loop from within the task.
//   auto headerClone = lsTask->getCloneOfOriginalBasicBlock(loopHeader);
//   IRBuilder<> entryBuilder(lsTask->getEntry());
//   entryBuilder.CreateBr(headerClone);
//
//   // Perform the iteration-chunking optimization
//   // this->rewireLoopToIterateChunks(LC, lsTask);
//
//   // Store final results to loop live-out variables. Note this occurs after
//   // all other code is generated. Propagated PHIs through the generated
//   // outer loop might affect the values stored
//   // this->generateCodeToStoreLiveOutVariables(LC, 0);
//   // this->invokeParallelizedLoop(LC);
//
//   /*
//    * Make PRVGs reentrant to avoid cache sharing.
//    */
//   // auto com = this->noelle.getCompilationOptionsManager();
//   // if (com->arePRVGsNonDeterministic()) {
//   //   errs() << "DOALL:  Make PRVGs reentrant\n";
//   //   this->makePRVGsReentrant();
//   // }
//
//   lsTask->getTaskBody()->print(errs() << "DOALL:  Final parallelized
//   loop:\n"); errs() << "\n";
// }

void terminateLCDs(Noelle &noelle, LoopContent *LC, TerminatorAnalysis &TA) {
  auto LS = LC->getLoopStructure();
  auto clauses = TA.getClausesOf(LS);

  errs() << "Terminator: Termination for loop " << LS->getID().value() << " ("
         << clauses.size() << " clauses)\n";

  // for (auto &clause : clauses) {
  //   auto *F = clause->getFunction();
  //   auto branchInst = LS->getPreHeader()->getTerminator();
  //   auto TCName = "TCValue." + to_string(terminationCounter);
  //   auto TCValue =
  //       CallInst::Create(F, clause->getCallArguments(), TCName, branchInst);
  //   StoreInst *SI = new StoreInst(TCValue, clause->getVariable(),
  //   branchInst); terminationCounter++;
  // }

  Winchester winchester(noelle);
}

} // namespace arcana::gino
