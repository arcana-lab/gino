#pragma once

#include "noelle/core/Noelle.hpp"
#include "noelle/core/Task.hpp"
#include "llvm/Pass.h"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

class LoopSliceTask : public Task {
public:
  LoopSliceTask(FunctionType *loopSliceTaskSignature, Module *module,
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

} // namespace arcana::gino
