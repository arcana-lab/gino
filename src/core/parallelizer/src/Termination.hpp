#ifndef __TERMINATION_HPP__
#define __TERMINATION_HPP__

#include <map>
#include <set>
#include <vector>

#include "TerminatorAnalysis.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/IR/Instructions.h"

namespace arcana::gino {

void terminateLCDs(noelle::LoopStructure *LS, TerminatorAnalysis &TA);

}

#endif
