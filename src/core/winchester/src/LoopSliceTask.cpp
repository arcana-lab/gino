#include "arcana/gino/core/LoopSliceTask.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

LoopSliceTask::LoopSliceTask(FunctionType *loopSliceTaskSignature,
                             Module *module, std::string &loopSliceTaskName)
    : Task(loopSliceTaskSignature, *module, loopSliceTaskName) {
  /*
   * Give each loop-slice task argument a name
   */
  auto argIter = this->F->arg_begin();
  this->taskMemoryPointerArg = (Value *)&*(argIter++);
  this->taskIndexArg = (Value *)&*(argIter++);
  this->LSTContextPointerArg = (Value *)&*(argIter++);
  this->invariantsPointerArg = (Value *)&*(argIter++);

  /*
   * NOELLE's task abstraction inserts a 'ret void' instruction
   * every time a task function is created.
   * A loop-slice task has a return type of i64, therefore,
   * remove this 'ret void' function after task creation.
   */
  this->getExit()->getFirstNonPHI()->eraseFromParent();

  return;
}

} // namespace arcana::gino
