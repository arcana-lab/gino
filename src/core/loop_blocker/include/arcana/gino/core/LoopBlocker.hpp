#pragma once

#include "llvm/IR/BasicBlock.h"
#include <noelle/core/Noelle.hpp>

namespace arcana::gino {

llvm::BasicBlock *blockLoop(noelle::LoopContent *LC, int numBlocks);

} // namespace arcana::gino
