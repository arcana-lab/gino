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
#include "llvm/ADT/iterator_range.h"

#include "arcana/noelle/core/NoellePass.hpp"
#include "arcana/gino/core/HeuristicsPass.hpp"

using namespace llvm;
using namespace arcana::gino;

HeuristicsPass::HeuristicsPass() {}

Heuristics HeuristicsPass::run(Module &M, ModuleAnalysisManager &MAM) {
  return Heuristics(MAM.getResult<NoellePass>(M));
}

// Next there is code to register your pass to "opt"
llvm::PassPluginLibraryInfo getPluginInfo() {
  return { LLVM_PLUGIN_API_VERSION,
           "Heuristics",
           LLVM_VERSION_STRING,
           [](PassBuilder &PB) {
             PB.registerAnalysisRegistrationCallback(
                 [](ModuleAnalysisManager &AM) {
                   AM.registerPass([&] { return NoellePass(); });
                   AM.registerPass([&] { return HeuristicsPass(); });
                 });
           } };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getPluginInfo();
}
