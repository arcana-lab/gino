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
#include "arcana/gino/core/DOALL.hpp"
#include "arcana/gino/core/DOALLTask.hpp"
#include "noelle/core/OutputSequenceSCC.hpp"

namespace arcana::gino {

bool DOALL::parallelizeOutput(LoopContent *LDI) {
  auto sccManager = LDI->getSCCManager();
  auto task = (DOALLTask *)this->tasks[0];

  auto chunkEnd =
      this->n.getProgram()->getFunction("GINO_DOALL_Scylax_ChunkEnd");
  auto taskEnd = this->n.getProgram()->getFunction("GINO_DOALL_Scylax_TaskEnd");

  if (chunkEnd == nullptr) {
    errs() << "Can't find DOALL ChunkEnd\n";
    abort();
  }
  if (taskEnd == nullptr) {
    errs() << "Can't find DOALL TaskEnd\n";
    abort();
  }

  bool modified = false;

  auto nonDOALLSCCs = sccManager->getSCCsWithLoopCarriedDependencies();

  for (auto sccInfo : nonDOALLSCCs) {
    auto outputSCC = dyn_cast<OutputSequenceSCC>(sccInfo);
    if (outputSCC == nullptr)
      continue;

    modified = true;

    if (this->verbose >= Verbosity::Maximal) {
      errs() << "DOALL:  Parallelizing an output sequence:\n";
    }

    for (auto originalI : outputSCC->getSCC()->getInstructions()) {
      auto I = dyn_cast<CallInst>(fetchCloneInTask(task, originalI));

      auto knownMaxLength = OutputSequenceSCC::getMaxNumberOfPrintedBytes(I);

      auto newPrintName = ParallelizationTechnique::
          parallelReplacementNameForInputOutputFunction(
              I->getCalledFunction()->getName(),
              knownMaxLength.has_value());
      auto newPrint = this->n.getProgram()->getFunction(newPrintName);

      if (newPrint == nullptr) {
        errs() << "DOALL: Can't find new print: " << newPrintName << "\n";
        abort();
      }

      IRBuilder<> replacer(I);
      std::vector<Value *> args = { task->scylaxPerThreadDataArg };
      if (knownMaxLength) {
        auto maxLengthType = newPrint->getFunctionType()->getParamType(1);
        args.push_back(ConstantInt::get(maxLengthType, knownMaxLength.value()));
      }

      args.insert(args.end(), I->data_operands_begin(), I->data_operands_end());
      auto newCall = replacer.CreateCall(newPrint, args);
      I->replaceAllUsesWith(newCall);
      I->eraseFromParent();

      if (this->verbose >= Verbosity::Maximal) {
        errs() << "DOALL:\t  " << I->getCalledFunction()->getName() << " --> "
               << newPrint->getName() << "\n";
      }
    }
  }

  if (modified) {
    IRBuilder<> chunkEndMarker(task->isChunkCompleted->getNextNode());
    auto asInt = chunkEndMarker.CreateIntCast(
        task->isChunkCompleted,
        chunkEnd->getFunctionType()->getParamType(0),
        false);
    chunkEndMarker.CreateCall(
        chunkEnd,
        ArrayRef<Value *>({ asInt, task->scylaxPerThreadDataArg }));

    IRBuilder<> taskEndMarker(&*(task->getExit()->getFirstInsertionPt()));
    taskEndMarker.CreateCall(
        taskEnd,
        ArrayRef<Value *>({ task->scylaxPerThreadDataArg }));
  }
  return modified;
}
} // namespace arcana::gino
