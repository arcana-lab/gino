/*
 * Copyright 2016 - 2023  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "arcana/gino/core/Winchester.hpp"

namespace arcana::gino {

Winchester::Winchester(
    Noelle *noelle, LoopNestAnalysis *lna, uint64_t nestID, LoopContent *lc,
    std::unordered_map<LoopContent *, std::unordered_set<Value *>>
        &loopToSkippedLiveIns,
    std::unordered_map<LoopContent *, std::unordered_map<Value *, int>>
        &loopToConstantLiveIns,
    std::unordered_map<int, int> &constantLiveInsArgIndexToIndex)
    : DOALL(*noelle), outputPrefix("Winchester Transformation: "),
      noelle(noelle), lna(lna), nestID(nestID), lc(lc),
      loopToSkippedLiveIns(loopToSkippedLiveIns),
      loopToConstantLiveIns(loopToConstantLiveIns),
      constantLiveInsArgIndexToIndex(constantLiveInsArgIndexToIndex) {
  errs() << "Terminator: Initializing Winchester\n";
}

bool Winchester::apply(LoopContent *lc, Heuristics *h) {
  errs() << "Terminator: Winchester apply()\n";

  this->createLoopSliceTaskFromOriginalLoop(lc);
  this->generateLoopClosure(lc);
  this->parameterizeLoopIterations(lc);

  return true;
}

void Winchester::createLoopSliceTaskFromOriginalLoop(LoopContent *lc) {
  /*
   * Get program from NOELLE
   */
  auto LoopID = lc->getLoopStructure()->getID().value();

  /*
   * Generate an empty loop-slice task for heartbeat execution.
   */
  auto name = std::string("LST_unnumbered");
  this->lsTask = new LoopSliceTask(this->loopSliceTaskSignature,
                                   this->noelle->getProgram(), name);

  /*
   * Initialize the loop-slice task with an entry and exit basic blocks.
   */
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(lc, {this->lsTask});

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
  this->cloneSequentialLoop(lc, 0);

  errs() << this->outputPrefix
         << "loop-slice task after cloning from original loop\n";
  errs() << *this->lsTask->getTaskBody() << "\n";

  return;
}

void Winchester::parameterizeLoopIterations(LoopContent *lc) {
  /*
   * Create an IRBuilder that insert before the terminator instruction
   * of the entry basic block of the loop-slice task.
   */
  auto lsTaskEntryBlock = this->lsTask->getEntry();
  IRBuilder<> iterationBuilder{lsTaskEntryBlock};
  iterationBuilder.SetInsertPoint(lsTaskEntryBlock->getTerminator());

  /*
   * Get the loop-governing induction variable.
   */
  auto ivm = lc->getInductionVariableManager();
  auto giv = ivm->getLoopGoverningInductionVariable();
  auto iv = giv->getInductionVariable()->getLoopEntryPHI();
  auto ivClone = cast<PHINode>(this->lsTask->getCloneOfOriginalInstruction(iv));
  this->inductionVariable = ivClone;

  /*
   * Adjust the start iteartion of the induction variable by
   * loading the value stored by the caller.
   */
  this->startIterationPointer = iterationBuilder.CreateInBoundsGEP(
      iterationBuilder.getInt64Ty(), this->lsTask->getLSTContextPointerArg(),
      // iterationBuilder.getInt64(this->lna->getLoopLevel(lc) *
      // this->valuesInCacheLine + this->startIteartionIndex),
      iterationBuilder.getInt64(this->startIteartionIndex),
      "start_iteration_pointer");
  this->startIteration = iterationBuilder.CreateLoad(
      iterationBuilder.getInt64Ty(), this->startIterationPointer,
      "start_iteration");
  this->inductionVariable->setIncomingValueForBlock(lsTaskEntryBlock,
                                                    this->startIteration);

  /*
   * Adjust the end iteration (a.k.a. exit condition) by
   * loading the value stored by the caller.
   */
  this->endIterationPointer = iterationBuilder.CreateInBoundsGEP(
      iterationBuilder.getInt64Ty(), this->lsTask->getLSTContextPointerArg(),
      // iterationBuilder.getInt64(this->lna->getLoopLevel(lc) *
      // this->valuesInCacheLine + this->endIterationIndex),
      iterationBuilder.getInt64(this->endIterationIndex),
      "end_iteration_pointer");
  this->endIteration =
      iterationBuilder.CreateLoad(iterationBuilder.getInt64Ty(),
                                  this->endIterationPointer, "end_iteration");

  /*
   * Replace the original exit condition value to
   * use the loaded end iteration value.
   */
  auto exitCmpInst = giv->getHeaderCompareInstructionToComputeExitCondition();
  this->exitCmpInstClone = dyn_cast<CmpInst>(
      this->lsTask->getCloneOfOriginalInstruction(exitCmpInst));
  auto exitConditionValue = giv->getExitConditionValue();
  auto firstOperand = exitCmpInst->getOperand(0);
  if (firstOperand == exitConditionValue) {
    this->exitCmpInstClone->setOperand(0, this->endIteration);
  } else {
    this->exitCmpInstClone->setOperand(1, this->endIteration);
  }

  errs() << this->outputPrefix
         << "loop-slice task after parameterizing loop iterations\n";
  errs() << *this->lsTask->getTaskBody() << "\n";

  return;
}

} // namespace arcana::gino
