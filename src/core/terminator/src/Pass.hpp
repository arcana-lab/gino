#pragma once

#include <string>
#include <vector>

#include "noelle/core/Noelle.hpp"

using namespace arcana::noelle;

namespace arcana::gino {

class TerminatorPass : public ModulePass {
public:
  static char ID;

  TerminatorPass();
  bool doInitialization(Module &M) override;
  bool runOnModule(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  std::string prefix;
};

} // namespace arcana::gino
