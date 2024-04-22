/*
 * Copyright 2023 - Federico Sossai, Simone Campanoni
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

#include "PlanInfo.hpp"

namespace arcana::gino {

PlanInfo::PlanInfo() : ModulePass{ ID } {
  return;
}

bool PlanInfo::runOnModule(Module &M) {
  auto &noelle = getAnalysis<Noelle>();
  auto verbosity = noelle.getVerbosity();

  errs() << "Parallelizer: PlanInfo: Start\n";
  /*
   * Fetch all the loops we want to parallelize.
   */
  auto forest = noelle.getLoopNestingForest();
  if (forest->getNumberOfLoops() == 0) {
    errs() << "Parallelizer: PlanInfo:    There is no loop to consider\n";
    delete forest;

    errs() << "Parallelizer: PlanInfo: Exit\n";
    return false;
  }

  /*
   * Collecting loops with a parallel plan
   */
  auto mm = noelle.getMetadataManager();
  std::map<int, LoopContent *> order2LoopContent;
  for (auto tree : forest->getTrees()) {
    auto collector = [&](LoopTree *n, uint32_t treeLevel) -> bool {
      auto LS = n->getLoop();
      if (!mm->doesHaveMetadata(LS, "gino.looporder")) {
        return false;
      }
      auto order = std::stoi(mm->getMetadata(LS, "gino.looporder"));
      auto optimizations = { LoopContentOptimization::MEMORY_CLONING_ID,
                             LoopContentOptimization::THREAD_SAFE_LIBRARY_ID };
      auto LC = noelle.getLoopContent(LS, optimizations);
      order2LoopContent[order] = LC;
      return false;
    };
    tree->visitPreOrder(collector);
  }

  errs() << "Parallelizer: PlanInfo: Number of loops: "
         << forest->getNumberOfLoops() << "\n";
  errs() << "Parallelizer: PlanInfo: Number of loops with a parallel plan: "
         << order2LoopContent.size() << "\n";

  const auto shouldPrint = [&](int order) {
    const auto &PH = this->printHeaders;
    return std::find(PH.begin(), PH.end(), order) != std::end(PH);
  };

  // If user didn't choose a subset we print all headers
  bool printAll = this->printHeaders.size() == 0;

  for (const auto &[order, LC] : order2LoopContent) {
    if (printAll || shouldPrint(order)) {
      errs() << "Parallelizer: PlanInfo:    Loop order: " << order << "\n";
      auto LS = LC->getLoopStructure();
      errs() << "Parallelizer: PlanInfo:    Function name: "
             << LS->getFunction()->getName().str() << "\n";
      errs() << *LS->getHeader() << "\n";
    }
  }
  errs() << "\n";

  return false;
}

} // namespace arcana::gino
