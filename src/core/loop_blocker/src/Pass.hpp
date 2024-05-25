#pragma once

#include <vector>

#include "noelle/core/Noelle.hpp"

using namespace arcana::noelle;

namespace arcana::gino {

class LoopBlockerPass : public ModulePass {
public:
  static char ID;

  LoopBlockerPass();
  bool doInitialization(Module &M) override;
  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  std::vector<int> loopIndexesWhiteList;
};

} // namespace arcana::gino
