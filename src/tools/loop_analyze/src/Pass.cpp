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

#include "arcana/gino/core/DOALL.hpp"

#include "Pass.hpp"

static cl::list<int> LoopIDs("loop-analyze-ids",
                             cl::ZeroOrMore,
                             cl::Hidden,
                             cl::CommaSeparated);

namespace arcana::gino {

bool LoopAnalyze::doInitialization(Module &M) {
  this->loopIDs = LoopIDs;
  return false;
}

void LoopAnalyze::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<NoellePass>();
  AU.addRequired<HeuristicsPass>();
}

LoopAnalyze::LoopAnalyze() : ModulePass{ ID }, prefix{ "LoopAnalyze: " } {
  return;
}

bool LoopAnalyze::runOnModule(Module &M) {
  auto &noelle = getAnalysis<NoellePass>().getNoelle();
  auto *heuristics = getAnalysis<HeuristicsPass>().getHeuristics(noelle);

  auto LSs = *noelle.getLoopStructures();

  auto isSelected = [&](int id) -> bool {
    for (auto v : this->loopIDs) {
      if (v == id) {
        return true;
      }
    }
    return false;
  };

  errs() << prefix << "Found " << LSs.size() << " loops\n";

  auto optimizations = { LoopContentOptimization::MEMORY_CLONING_ID,
                         LoopContentOptimization::THREAD_SAFE_LIBRARY_ID };

  for (auto *LS : LSs) {
    auto ID = LS->getID().value();
    if (!isSelected(ID)) {
      continue;
    }
    errs() << prefix << "Loop \e[1m" << ID << "\e[0m: DOALL: ";

    DOALL doall(noelle);
    auto LC = noelle.getLoopContent(LS, optimizations);
    auto sccManager = LC->getSCCManager();

    if (doall.canBeAppliedToLoop(LC, heuristics)) {
      errs() << "\e[32mYes\e[0m\n";
    } else {
      errs() << "\e[31mNo\e[0m\n";
      size_t depCounter = 0;
      auto SCCs = DOALL::getSCCsThatBlockDOALLToBeApplicable(LC, noelle);
      for (auto scc : SCCs) {
        auto sccInfo = sccManager->getSCCAttrs(scc);
        if (auto loopCarriedSCC = dyn_cast<LoopCarriedSCC>(sccInfo)) {
          vector<pair<Value *, Value *>> LCDs;
          // dependence set to dependence vector (I want indexes)
          for (auto dep : loopCarriedSCC->getLoopCarriedDependences()) {
            if (isa<ControlDependence<Value, Value>>(dep)) {
              continue;
            }
            auto src = dep->getSrc();
            auto dst = dep->getDst();
            LCDs.push_back({ src, dst });
            depCounter++;
          }

          set<size_t> toSkip;
          for (size_t i = 0; i < LCDs.size(); i++) {
            if (toSkip.find(i) != toSkip.end()) {
              continue;
            }
            auto src = LCDs[i].first;
            auto dst = LCDs[i].second;

            if (src == dst) {
              errs() << prefix << "\t \u21bb" << *src << "\n";
            } else {
              bool isSelf = false;
              for (size_t j = 0; j < LCDs.size(); j++) {
                auto otherSrc = LCDs[j].first;
                auto otherDst = LCDs[j].second;
                if (src == otherDst && dst == otherSrc) {
                  toSkip.insert(j);
                  isSelf = true;
                  break;
                }
              }

              if (isSelf) {
                errs() << prefix << "\t\u250f\u2192" << *src << "\n";
              } else {
                errs() << prefix << "\t\u250f\u2501" << *src << "\n";
              }
              errs() << prefix << "\t\u2517\u2192" << *dst << "\n";
            }
          }
        }
      }
      errs() << prefix << "Loop \e[1m" << ID
             << "\e[0m: LCDs count: " << depCounter << "\n";
    }
  }

  return false;
}

char LoopAnalyze::ID = 0;
static RegisterPass<LoopAnalyze> X(
    "LoopAnalyze",
    "Analyze parallelizability of a set of loops");

} // namespace arcana::gino
