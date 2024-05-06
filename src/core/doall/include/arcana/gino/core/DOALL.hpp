/*
 * Copyright 2016 - 2024  Angelo Matni, Sophia Boksenbaum, Simone Campanoni
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
#ifndef NOELLE_SRC_TOOLS_DOALL_H_
#define NOELLE_SRC_TOOLS_DOALL_H_

#include "noelle/core/IVStepperUtility.hpp"
#include "noelle/core/Noelle.hpp"

#include "arcana/gino/core/DOALLTask.hpp"
#include "arcana/gino/core/HeuristicsPass.hpp"
#include "arcana/gino/core/ParallelizationTechnique.hpp"

namespace arcana::gino {

class DOALL : public ParallelizationTechnique {
public:
  /*
   * Methods
   */
  DOALL(Noelle &noelle);

  bool apply(LoopContent *LDI, Heuristics *h) override;

  bool canBeAppliedToLoop(LoopContent *LDI, Heuristics *h) const override;

  uint32_t getMinimumNumberOfIdleCores(void) const override;

  std::string getName(void) const override;

  Transformation getParallelizationID(void) const override;

  static std::set<SCC *> getSCCsThatBlockDOALLToBeApplicable(LoopContent *LDI,
                                                             Noelle &par);

protected:
  bool enabled;
  Function *taskDispatcher;
  Noelle &n;
  std::map<PHINode *, std::set<Instruction *>> IVValueJustBeforeEnteringBody;

  virtual void invokeParallelizedLoop(LoopContent *LDI,
                                      bool loopHasParallelizedOutput);

  /*
   * DOALL specific generation
   */
  void rewireLoopToIterateChunks(LoopContent *LDI, DOALLTask *task);
  bool replaceOutputSequences(LoopContent *LDI);

  /*
   * Interface
   */
  BasicBlock *getBasicBlockExecutedOnlyByLastIterationBeforeExitingTask(
      LoopContent *LDI,
      uint32_t taskIndex,
      BasicBlock &bb) override;
};

} // namespace arcana::gino

#endif // NOELLE_SRC_TOOLS_DOALL_H_
