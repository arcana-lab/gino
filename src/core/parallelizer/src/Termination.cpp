#include <string>

#include "TerminatorAnalysis.hpp"
#include "arcana/gino/core/DOALL.hpp"
#include "noelle/core/Noelle.hpp"
#include "noelle/core/Task.hpp"
#include "llvm/IR/Instructions.h"

using namespace std;
using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

static int terminationCounter = 0;

class LoopSliceTask : public Task {
public:
  LoopSliceTask(Module *module, FunctionType *loopSliceTaskSignature,
                std::string &loopSliceTaskName);

  inline Value *getTaskMemoryPointerArg() {
    return this->taskMemoryPointerArg;
  };
  inline Value *getTaskIndexArg() { return this->taskIndexArg; };
  inline Value *getLSTContextPointerArg() {
    return this->LSTContextPointerArg;
  };
  inline Value *getInvariantsPointerArg() {
    return this->invariantsPointerArg;
  };

private:
  Value *taskMemoryPointerArg;
  Value *taskIndexArg;
  Value *LSTContextPointerArg;
  Value *invariantsPointerArg;
};

class Winchester : public DOALL {
public:
  Winchester(Noelle &noelle) : DOALL(noelle) {}

  void sliceLoop(Noelle &noelle, LoopStructure *LS);
  FunctionType *loopSliceTaskSignature;
};

void Winchester::sliceLoop(Noelle &noelle, LoopStructure *LS) {
  auto &M = *noelle.getProgram();
  auto LC = noelle.getLoopContent(LS);

  auto tm = noelle.getTypesManager();
  std::vector<Type *> taskMemoryStructFieldTypes = {
      tm->getIntegerType(64), // starting_level
      tm->getIntegerType(64), // chunk_size
      tm->getIntegerType(64), // remaining_chunk_size
      tm->getIntegerType(64), // polling_count
  };

  auto taskMemoryStructType = StructType::create(
      noelle.getProgramContext(), taskMemoryStructFieldTypes, "task_memory_t");

  std::vector<Type *> loopSliceTaskSignatureTypes{
      PointerType::getUnqual(taskMemoryStructType),   // *tmem
      tm->getIntegerType(64),                         // task_index
      PointerType::getUnqual(tm->getIntegerType(64)), // *cxts
      PointerType::getUnqual(tm->getIntegerType(64))  // *invariants
  };

  this->loopSliceTaskSignature =
      FunctionType::get(tm->getIntegerType(64),
                        ArrayRef<Type *>(loopSliceTaskSignatureTypes), false);

  /*
   * Generate an empty loop-slice task for heartbeat execution.
   */
  auto lsTask =
      new Task(this->loopSliceTaskSignature, M, std::string("LST_0")
               // std::string("LST_").append(this->lna->getLoopIDString(LC))
      );

  /*
   * Initialize the loop-slice task with an entry and exit basic blocks.
   */
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(LC, {lsTask});

  /*
   * NOELLE's ParallelizationTechnique abstraction creates two basic blocks
   * in the original loop function.
   * For heartbeat, these two basic blocks are not needed.
   */
  this->getParLoopEntryPoint()->eraseFromParent();
  this->getParLoopExitPoint()->eraseFromParent();

  /*
   * Clone the loop into the loop-slice task.
   */
  this->cloneSequentialLoop(LC, 0);

  if (true) {
    errs() << "loopSlice(): loop-slice task after cloning from original loop\n";
    errs() << *lsTask->getTaskBody() << "\n";
  }

  return;
}

void terminateLCDs(Noelle &noelle, LoopStructure *LS, TerminatorAnalysis &TA) {
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

  Winchester winch(noelle);
  winch.sliceLoop(noelle, LS);
}

} // namespace arcana::gino
