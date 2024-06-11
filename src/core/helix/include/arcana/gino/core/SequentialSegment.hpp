/*
 * Copyright 2016 - 2019  Angelo Matni, Simone Campanoni
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
#ifndef NOELLE_SRC_TOOLS_HELIX_SEQUENTIALSEGMENT_H_
#define NOELLE_SRC_TOOLS_HELIX_SEQUENTIALSEGMENT_H_

#include "llvm/ADT/iterator_range.h"
#include "arcana/noelle/core/SCCDAGPartition.hpp"
#include "arcana/gino/core/HELIX.hpp"

namespace arcana::gino {

class SequentialSegment {
public:
  SequentialSegment(Noelle &noelle,
                    LoopContent *LDI,
                    DataFlowResult *reachabilityDFR,
                    SCCSet *sccs,
                    int32_t ID,
                    Verbosity verbosity,
                    const std::string &prefixString);

  void forEachEntry(std::function<void(Instruction *justAfterEntry)> whatToDo);

  void forEachExit(std::function<void(Instruction *justBeforeExit)> whatToDo);

  int32_t getID(void);

  iterator_range<unordered_set<SCC *>::iterator> getSCCs(void);

  std::unordered_set<Instruction *> getInstructions(void);

private:
  std::set<Instruction *> entries;
  std::set<Instruction *> exits;
  SCCSet *sccs;
  int32_t ID;
  Verbosity verbosity;

  void determineEntryAndExitFrontier(
      LoopContent *LDI,
      DominatorSummary *DS,
      DataFlowResult *dfr,
      std::unordered_set<Instruction *> &ssInstructions);

  /*
   * TODO: Remove this once determineEntryAndExitFrontier is proven to work with
   * more extensive testing
   */
  void determineEntriesAndExits(
      LoopContent *LDI,
      DataFlowResult *dfr,
      std::unordered_set<Instruction *> &ssInstructions);

  Instruction *getFrontierInstructionThatDoesNotSplitPHIs(
      Instruction *originalBarrierInst);

  std::unordered_map<Instruction *, std::unordered_set<Instruction *>>
  computeBeforeInstructionMap(LoopContent *LDI, DataFlowResult *dfr);

  void printSCCInfo(LoopContent *LDI,
                    std::unordered_set<Instruction *> &ssInstructions,
                    const std::string &prefixString);

  void classifyEntriesAndExitsUsingReachabilityResults(
      LoopStructure *loopContainingSSInstructions,
      DataFlowResult *dfr,
      std::unordered_set<Instruction *> &ssInstructions);
};

} // namespace arcana::gino

#endif // NOELLE_SRC_TOOLS_HELIX_SEQUENTIALSEGMENT_H_
