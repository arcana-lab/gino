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
#ifndef GINO_SRC_CORE_WINCHESTER_H
#define GINO_SRC_CORE_WINCHESTER_H

#include "noelle/core/IVStepperUtility.hpp"
#include "noelle/core/LoopContent.hpp"
#include "noelle/core/Noelle.hpp"

#include "arcana/gino/core/DOALL.hpp"
#include "arcana/gino/core/LoopNestAnalysis.hpp"
#include "arcana/gino/core/LoopSliceTask.hpp"

namespace arcana::gino {

class Winchester : public DOALL {
public:
  Winchester(Noelle *noelle, LoopNestAnalysis *lna, uint64_t nestID,
             LoopContent *lc,
             std::unordered_map<LoopContent *, std::unordered_set<Value *>>
                 &loopToSkippedLiveIns,
             std::unordered_map<LoopContent *, std::unordered_map<Value *, int>>
                 &loopToConstantLiveIns,
             std::unordered_map<int, int> &constantLiveInsArgIndexToIndex);

  void parameterizeLoopIterations(LoopContent *lc);

  void createLoopSliceTaskFromOriginalLoop(LoopContent *lc);
  void generateLoopClosure(LoopContent *lc);

  bool apply(LoopContent *lc, Heuristics *h) override;

  void initializeEnvironmentBuilder(
      LoopContent *LDI,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          shouldThisVariableBeReduced,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          shouldThisVariableBeSkipped,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          isConstantLiveInVariable);

  void allocateNextLevelReducibleEnvironmentInsideTask(LoopContent *lc,
                                                       int taskIndex);
  void generateCodeToStoreLiveOutVariables(LoopContent *lc, int taskIndex);
  void setReducableVariablesToBeginAtIdentityValue(LoopContent *lc,
                                                   int taskIndex);
  void generateCodeToLoadLiveInVariables(LoopContent *lc, int taskIndex);
  void initializeLoopEnvironmentUsers();

  // HBTVerbosity verbose;
  StringRef outputPrefix;
  Noelle *noelle;
  // HeartbeatRuntimeManager *hbrm;
  LoopNestAnalysis *lna;
  uint64_t nestID;
  LoopContent *lc;

  uint64_t valuesInCacheLine;
  uint64_t startIteartionIndex;
  uint64_t endIterationIndex;
  uint64_t liveInEnvIndex;
  uint64_t liveOutEnvIndex;
  FunctionType *loopSliceTaskSignature;
  LoopSliceTask *lsTask;

  Value *LSTContextBitCastInst;
  PHINode *inductionVariable;
  CmpInst *exitCmpInstClone;
  Value *startIterationPointer;
  Value *startIteration;
  Value *endIterationPointer;
  Value *endIteration;
  // CallInst *promotionHandlerCallInst;

  std::unordered_map<LoopContent *, std::unordered_set<Value *>>
      &loopToSkippedLiveIns;
  std::unordered_map<LoopContent *, std::unordered_map<Value *, int>>
      &loopToConstantLiveIns;
  std::unordered_map<int, int> &constantLiveInsArgIndexToIndex;
  std::unordered_map<uint32_t, Value *> liveOutVariableToAccumulatedPrivateCopy;
};

} // namespace arcana::gino

#endif // GINO_SRC_CORE_WINCHESTER_H
