#include "TerminatorAnalysis.hpp"

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <vector>

#include "noelle/core/LoopCarriedUnknownSCC.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/IR/Instructions.h"

using namespace std;
using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

TerminatorAnalysis::TerminatorAnalysis(Noelle &noelle)
    : DependenceAnalysis("Terminator"), noelle(noelle) {}

TerminatorAnalysis::~TerminatorAnalysis() {
  for (auto *C : clauses_) {
    delete C;
  }
}

void TerminatorAnalysis::run(set<LoopStructure *> &LSs) {
  auto targetLSs = findTargetLoops(LSs);
  resolveClauses(targetLSs);
}

void TerminatorAnalysis::run() {
  auto LSs_vector = noelle.getLoopStructures();
  set<LoopStructure *> LSs(LSs_vector->begin(), LSs_vector->end());
  run(LSs);
  delete LSs_vector;
}

bool TerminatorAnalysis::canThisDependenceBeLoopCarried(Dependence *LCD,
                                                        LoopStructure &LS) {
  auto coverage = getCoverage(LCD);
  if (coverage != NONE) {
    // errs() << "Terminator: Analysis: Query: Terminated\n";
    return false;
  }
  if (coverage != NONE) {
    // errs() << "Terminator: Analysis: Query: Let live ("
    //        << coverageToString(coverage) << ")\n";
    printDependence(LCD);
  }
  return true;
}

void TerminatorAnalysis::printDependence(const Dependence *LCD) const {
  auto srcValue = LCD->getSrcNode()->getT();
  auto dstValue = LCD->getDstNode()->getT();
  errs() << "Terminator: Analysis: Dependence: [src] " << *srcValue << "\n";
  errs() << "Terminator: Analysis: Dependence: [dst] " << *dstValue << "\n";
}

bool TerminatorAnalysis::isLDTCBegin(const Instruction *I) const {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto callee = CI->getCalledFunction();
    if (callee) {
      if (callee->getName().startswith("_Z15__dt_ldtc_begin")) {
        return true;
      }
    }
  }
  return false;
}

bool TerminatorAnalysis::isLDTCEnd(const Instruction *I) const {
  if (auto *CI = dyn_cast<CallInst>(I)) {
    auto callee = CI->getCalledFunction();
    if (callee) {
      if (callee->getName().startswith("_Z13__dt_ldtc_end")) {
        return true;
      }
    }
  }
  return false;
}

bool TerminatorAnalysis::isLDTC(const Instruction *I) const {
  return isLDTCBegin(I) || isLDTCEnd(I);
}

set<Instruction *> TerminatorAnalysis::getPragmasInLoop(LoopStructure *LS,
                                                        LoopForest *LF) const {
  set<Instruction *> pragmas;
  for (auto I : LS->getInstructions()) {
    if (isLDTC(I)) {
      // Pragmas are always referred to the innermost loop that contain them
      if (LF->getInnermostLoopThatContains(I)->getLoop()->getHeader() ==
          LS->getHeader()) {
        pragmas.insert(I);
      }
    }
  }
  return pragmas;
}

set<LoopStructure *>
TerminatorAnalysis::findTargetLoops(set<LoopStructure *> &LSs) {
  auto PDG = noelle.getProgramDependenceGraph();
  set<LoopStructure *> targetLSs;

  errs() << "Terminator: Analysis: Info: " << LSs.size() << " input loops\n";

  for (auto LS : LSs) {
    auto LC = noelle.getLoopContent(LS);
    auto sccManager = LC->getSCCManager();
    auto SCCDAG = sccManager->getSCCDAG();

    for (auto sccNode : SCCDAG->getSCCs()) {
      auto genericSCC = sccManager->getSCCAttrs(sccNode);
      if (auto LCU = dyn_cast<LoopCarriedUnknownSCC>(genericSCC)) {
        auto LCDs = LCU->getLoopCarriedDependences();
        // Filtering out control dependences
        for (auto it = LCDs.begin(); it != LCDs.end();) {
          auto dep = *it;
          if (isa<ControlDependence<Value, Value>>(dep)) {
            it = LCDs.erase(it);
          } else {
            ++it;
          }
        }
        unknownLCDs_.insert(LCDs.begin(), LCDs.end());
        candidateLSs_.insert(LS);

        // for (auto LCD : LCDs) {
        //   errs() << "Terminator: Analysis: Dependence: In "
        //          << LS->getFunction()->getName() << "\n";
        //   printDependence(LCD);
        // }
      }
    }
  }

  errs() << "Terminator: Analysis: Info: Found " << unknownLCDs_.size()
         << " candidate LCDs\n";
  errs() << "Terminator: Analysis: Info: Found " << candidateLSs_.size()
         << " loops with LCDs\n";

  auto LF = noelle.getLoopNestingForest();

  vector<int> targetLoopIDs;
  for (auto LS : candidateLSs_) {
    auto pragmas = getPragmasInLoop(LS, LF);
    if (pragmas.size() > 0) {
      targetLSs.insert(LS);
      loopToPragmas_[LS] = pragmas;
      targetLoopIDs.push_back(LS->getID().value());
    }
  }
  errs() << "Terminator: Analysis: Info: Found " << targetLoopIDs.size()
         << " target loops. IDs = { ";
  for (auto &id : targetLoopIDs) {
    errs() << id << " ";
  }
  errs() << "}\n";

  return targetLSs;
}

bool TerminatorAnalysis::isUnmatched(Instruction *begin) const {
  return matchedBegins_.find(begin) == matchedBegins_.end();
}

Instruction *TerminatorAnalysis::findUnmatchedBegin(BasicBlock *BB) const {
  stack<Instruction *> begins;
  for (auto &I : *BB) {
    if (isLDTCBegin(&I) && isUnmatched(&I)) {
      begins.push(&I);
    } else if (isLDTCEnd(&I)) {
      if (!begins.empty()) {
        begins.pop();
      }
    }
  }
  if (begins.empty()) {
    return nullptr;
  }
  return begins.top();
}

Instruction *
TerminatorAnalysis::findMatchingBeginSingleBlock(Instruction *end) const {
  stack<Instruction *> ends;
  ends.push(end);
  auto BB = end->getParent();
  auto rit = BB->rbegin();
  while (&*rit != end)
    rit++;
  for (; rit != BB->rend(); rit++) {
    auto &I = *rit;
    if (isLDTCBegin(&I) && isUnmatched(&I)) {
      if (ends.top() == end) {
        return &I;
      } else {
        ends.pop();
      }
    } else if (isLDTCEnd(&I)) {
      ends.push(&I);
    }
  }
  return nullptr;
}

set<Instruction *>
TerminatorAnalysis::findMatchingBegin(Instruction *end,
                                      Instruction **beginFound) {
  auto F = end->getParent()->getParent();
  auto DS = noelle.getDominators(F);
  auto &DT = DS->DT;

  auto endBB = end->getParent();
  BasicBlock *beginBB = nullptr;
  *beginFound = nullptr;

  set<Instruction *> region;
  auto addRangeToRegion = [&](auto from, auto to) {
    auto it = from;
    while (it != to) {
      region.insert(&*it);
      it++;
    }
  };

  if (auto begin = findMatchingBeginSingleBlock(end)) {
    auto itFrom = endBB->begin();
    while (&*itFrom != begin)
      itFrom++;
    auto itTo = itFrom;
    while (&*itTo != end)
      itTo++;
    addRangeToRegion(++itFrom, itTo);
    *beginFound = begin;
    return region;
  }

  queue<BasicBlock *> q;
  set<BasicBlock *> selected;
  auto alreadySelected = [&](BasicBlock *BB) {
    return selected.find(BB) != selected.end();
  };
  q.push(endBB);

  while (!q.empty()) {
    auto current = q.front();
    q.pop();

    for (auto BB : predecessors(current)) {
      auto begin = findUnmatchedBegin(BB);
      if (begin == nullptr) {
        if (!alreadySelected(BB)) {
          selected.insert(BB);
          q.push(BB);
        }
      } else {
        if (beginBB != nullptr) {
          // In case another potential beginBB was found before, it must be the
          // same one that we just rediscovered. This happens for example with
          // diamond-shaped CFGs with a `begin` at the top and an `end` at the
          // bottom.
          assert(beginBB == BB);
        }
        beginBB = BB;
        *beginFound = begin;
      }
    }
  }

  for (auto BB : selected) {
    addRangeToRegion(BB->begin(), BB->end());
  }

  // considering all instructions after `begin` in its BB
  auto it1 = beginBB->begin();
  auto begin = findUnmatchedBegin(beginBB);
  assert(begin != nullptr);
  while (&*it1 != begin)
    it1++;
  addRangeToRegion(++it1, beginBB->end());

  // considering all instruction before 'end' in its BB
  auto it2 = endBB->begin();
  while (&*it2 != end)
    it2++;
  addRangeToRegion(endBB->begin(), it2);

  *beginFound = begin;
  return region;
}

void TerminatorAnalysis::resolveClauses(LoopStructure *LS) {
  auto &pragmas = loopToPragmas_[LS];

  // Find `end` pragmas
  set<Instruction *> ends;
  for (auto I : pragmas) {
    if (isLDTCEnd(I)) {
      ends.insert(I);
    }
  }

  set<Clause *> foundClauses;
  for (auto end : ends) {
    Instruction *begin;
    auto region = findMatchingBegin(end, &begin);
    matchedBegins_.insert(begin);
    auto clause = new Clause(begin, end);
    foundClauses.insert(clause);
    clauseToInsts_[clause] = region;
    for (auto I : region) {
      instToClause_[I] = clause;
    }
  }
  loopToClauses_[LS] = foundClauses;
  clauses_.insert(foundClauses.begin(), foundClauses.end());
  errs() << "Terminator: Analysis: Info: Found " << clauses_.size()
         << " clauses\n";
}

void TerminatorAnalysis::resolveClauses(set<LoopStructure *> &targetLSs) {
  for (auto *LS : targetLSs) {
    resolveClauses(LS);
  }
}

set<Clause *> TerminatorAnalysis::getClausesOf(LoopStructure *LS) {
  auto it = loopToClauses_.find(LS);
  if (it != loopToClauses_.end()) {
    return it->second;
  }
  return {};
}

void TerminatorAnalysis::printClauses() const {
  for (auto C : clauses_) {
    C->print();
  }
}

set<const Clause *> TerminatorAnalysis::canBeTerminated(Dependence *LCD) const {
  auto none = instToClause_.end();
  auto srcValue = cast<Instruction>(LCD->getSrcNode()->getT());
  auto dstValue = cast<Instruction>(LCD->getDstNode()->getT());
  auto srcClause = instToClause_.find(srcValue);
  auto dstClause = instToClause_.find(dstValue);

  if (srcClause == none && dstClause == none) {
    return {};
  } else if (srcClause != none && dstClause == none) {
    return {srcClause->second};
  } else if (srcClause == none && dstClause != none) {
    return {dstClause->second};
  } else if (srcClause != none && dstClause != none) {
    if (srcClause->second == dstClause->second) {
      return {srcClause->second};
    } else {
      return {srcClause->second, dstClause->second};
    }
  }
}

Coverage TerminatorAnalysis::getCoverage(Dependence *LCD) {
  const auto none = instToClause_.end();
  auto srcValue = cast<Instruction>(LCD->getSrcNode()->getT());
  auto dstValue = cast<Instruction>(LCD->getDstNode()->getT());
  auto srcClause = instToClause_.find(srcValue);
  auto dstClause = instToClause_.find(dstValue);

  Coverage coverage;
  if (srcClause == none && dstClause == none) {
    return NONE;
  }
  if (srcClause != none && dstClause == none) {
    return SRC_ONLY;
  }
  if (srcClause == none && dstClause != none) {
    return DST_ONLY;
  }
  if (srcClause != none && dstClause != none) {
    if (srcClause->second == dstClause->second) {
      return FULL;
    }
    return CROSS;
  }
}

string TerminatorAnalysis::coverageToString(Coverage coverage) {
  switch (coverage) {
  case NONE:
    return "uncovered";
    break;
  case SRC_ONLY:
    return "source-only-covered";
    break;
  case DST_ONLY:
    return "destination-only-covered";
    break;
  case CROSS:
    return "cross-covered";
    break;
  case FULL:
    return "cross-covered";
    break;
  }
}

void TerminatorAnalysis::printCoverage() {
  // here we use being `covered` meaning that an instruction
  // is in a region of code that belongs to a clause
  int notCovered = 0;
  int onlySrcCovered = 0;
  int onlyDstCovered = 0;
  int crossCovered = 0;
  int fullyCovered = 0;

  for (auto LCD : unknownLCDs_) {
    auto coverage = getCoverage(LCD);
    switch (coverage) {
    case NONE:
      notCovered++;
      break;
    case SRC_ONLY:
      onlySrcCovered++;
      break;
    case DST_ONLY:
      onlyDstCovered++;
      break;
    case CROSS:
      crossCovered++;
      break;
    case FULL:
      fullyCovered++;
      break;
    }
    if (coverage != FULL) {
      errs() << "Terminator: Analysis: Dependence: Found "
             << coverageToString(coverage) << " LCD\n";
      printDependence(LCD);
    }
  }

  errs() << "Terminator: Analysis: Info: Found " << notCovered
         << " uncovered LCDs\n";
  errs() << "Terminator: Analysis: Info: Found " << fullyCovered
         << " fully-covered LCDs\n";
  errs() << "Terminator: Analysis: Info: Found " << crossCovered
         << " cross-covered LCDs\n";
  errs() << "Terminator: Analysis: Info: Found " << onlySrcCovered
         << " source-only-covered LCDs\n";
  errs() << "Terminator: Analysis: Info: Found " << onlyDstCovered
         << " destination-only-covered LCDs\n";
}

void TerminatorAnalysis::sanityChecks() {
  // `begin` and `end` instructions must belong to one and only one clause
  set<Instruction *> beginSeen;
  set<Instruction *> endSeen;
  for (auto C : clauses_) {
    bool duplicatedBegin = !beginSeen.insert(C->getBegin()).second;
    assert(!duplicatedBegin);
    bool duplicatedEnd = !endSeen.insert(C->getEnd()).second;
    assert(!duplicatedEnd);
  }
}

} // namespace arcana::gino
