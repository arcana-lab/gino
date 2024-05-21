/*
 * Copyright 2024  Sophia Boksenbaum
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
#include "noelle/core/OutputSequenceSCC.hpp"
#include "arcana/gino/core/HELIX.hpp"
#include "arcana/gino/core/HELIXTask.hpp"

namespace arcana::gino {

bool HELIX::parallelizeOutput(LoopContent *LDI) {

  auto sccManager = LDI->getSCCManager();
  auto task = (HELIXTask *)this->tasks[0];

  auto iterEnd =
      this->noelle.getProgram()->getFunction("GINO_HELIX_Scylax_IterEnd");
  auto taskEnd =
      this->noelle.getProgram()->getFunction("GINO_HELIX_Scylax_TaskEnd");

  if (iterEnd == nullptr) {
    errs() << "Can't find HELIX IterEnd\n";
    abort();
  }
  if (taskEnd == nullptr) {
    errs() << "Can't find HELIX TaskEnd\n";
    abort();
  }

  bool modified = false;

  auto LCDSCCs = sccManager->getSCCsWithLoopCarriedDependencies();
  for (auto sccInfo : LCDSCCs) {
    auto outputSCC = dyn_cast<OutputSequenceSCC>(sccInfo);
    if (outputSCC == nullptr)
      continue;

    modified = true;

    if (this->verbose >= Verbosity::Maximal) {
      errs() << "HELIX: Parallelizing an output sequence:\n";
    }

    for (auto originalI : outputSCC->getSCC()->getInstructions()) {
      auto I = dyn_cast<CallInst>(fetchCloneInTask(task, originalI));

      auto knownMaxLength = OutputSequenceSCC::getMaxNumberOfPrintedBytes(I);

      auto newPrintName = ParallelizationTechnique::
          parallelReplacementNameForInputOutputFunction(
              I->getCalledFunction()->getName(),
              knownMaxLength.has_value());
      auto newPrint = this->noelle.getProgram()->getFunction(newPrintName);

      if (newPrint == nullptr) {
        errs() << "HELIX: Can't find new print: " << newPrintName << "\n";
        abort();
      }

      IRBuilder<> replacer(I);
      std::vector<Value *> args = { task->scylaxDataArg };
      if (knownMaxLength) {
        auto maxLengthType = newPrint->getFunctionType()->getParamType(1);
        args.push_back(ConstantInt::get(maxLengthType, knownMaxLength.value()));
      }

      args.insert(args.end(), I->data_operands_begin(), I->data_operands_end());
      auto newCall = replacer.CreateCall(newPrint, args);
      I->replaceAllUsesWith(newCall);
      I->eraseFromParent();

      if (this->verbose >= Verbosity::Maximal) {
        errs() << "HELIX:\t  " << I->getCalledFunction()->getName() << " --> "
               << newPrint->getName() << "\n";
      }
    }
  }

  if (modified) {
    for (auto &latchO : LDI->getLoopStructure()->getLatches()) {
      auto latch = task->getCloneOfOriginalBasicBlock(latchO);
      IRBuilder<> iterEndMarker(latch->getTerminator());
      iterEndMarker.CreateCall(iterEnd,
                               ArrayRef<Value *>({ task->scylaxDataArg }));
    }

    IRBuilder<> taskEndMarker(&*(task->getExit()->getFirstInsertionPt()));
    taskEndMarker.CreateCall(taskEnd,
                             ArrayRef<Value *>({ task->scylaxDataArg }));
  }
  return modified;
}
} // namespace arcana::gino
