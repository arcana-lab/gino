/*
 * Copyright 2022 Kevin McAfee, Simone Campanoni
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
#include "arcana/gino/tools/TimeSaved.hpp"
#include "arcana/noelle/core/NoellePass.hpp"

namespace arcana::gino {

TimeSaved::TimeSaved() : forceParallelization{ true } {
  return;
}

PreservedAnalyses TimeSaved::run(Module &M, ModuleAnalysisManager &MAM) {
  errs() << "TimeSaved: Start\n";

  /*
   * Fetch the outputs of the passes we rely on.
   */
  auto &noelle = MAM.getResult<NoellePass>(M);

  /*
   * Fetch the profiles.
   */
  auto profiles = noelle.getProfiles();

  /*
   * Fetch all the loops we want to parallelize.
   */
  errs() << "TimeSaved:  Fetching the program loops\n";
  auto noOverrideDecision = [](LoopStructure *l) -> bool { return false; };
  auto loops =
      noelle.getLoopStructures(noelle.getMinimumHotness(), noOverrideDecision);
  auto forest = noelle.organizeLoopsInTheirNestingForest(*loops);
  if (forest->getNumberOfLoops() == 0) {
    errs() << "TimeSaved:    There is no loop to consider\n";

    /*
     * Free the memory.
     */
    delete forest;

    errs() << "TimeSaved: Exit\n";
    return PreservedAnalyses::all();
  }
  errs() << "TimeSaved:    There are " << forest->getNumberOfLoops()
         << " loops in the program we are going to consider\n";

  /*
   * Collect metrics for time saved by parallelizing.
   */
  uint64_t programMaxTimeSaved = 0;
  uint64_t programMaxTimeSavedWithDOALLOnly = 0;
  for (auto tree : forest->getTrees()) {

    /*
     * Select the loops to parallelize.
     */
    uint64_t maxTimeSaved = 0;
    uint64_t maxTimeSavedWithDOALLOnly = 0;
    auto loopsToParallelize =
        this->selectTheOrderOfLoopsToParallelize(noelle,
                                                 profiles,
                                                 tree,
                                                 maxTimeSaved,
                                                 maxTimeSavedWithDOALLOnly);
    programMaxTimeSaved += maxTimeSaved;
    programMaxTimeSavedWithDOALLOnly += maxTimeSavedWithDOALLOnly;

    /*
     * Free the memory.
     */
    for (auto loop : loopsToParallelize) {
      delete loop;
    }
  }

  /*
   * Print statistics.
   */
  auto savedTimeTotal = ((double)programMaxTimeSaved)
                        / ((double)profiles->getTotalInstructions());
  savedTimeTotal *= 100;
  errs() << "TimeSaved:   Maximum time saved = " << savedTimeTotal << "% ("
         << programMaxTimeSaved << ")\n";
  savedTimeTotal = ((double)programMaxTimeSavedWithDOALLOnly)
                   / ((double)profiles->getTotalInstructions());
  savedTimeTotal *= 100;
  errs()
      << "TimeSaved:   Maximum time saved with DOALL only = " << savedTimeTotal
      << "% (" << programMaxTimeSavedWithDOALLOnly << ")\n";

  errs() << "TimeSaved: Exit\n";
  return PreservedAnalyses::all();
}

} // namespace arcana::gino
