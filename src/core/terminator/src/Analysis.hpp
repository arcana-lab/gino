#ifndef __TERMINATOR_ANALYSIS_HPP__
#define __TERMINATOR_ANALYSIS_HPP__

#include <map>
#include <set>
#include <vector>

#include "Clause.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/IR/Instructions.h"

namespace arcana::gino {

enum Coverage {
  NONE = 0b00001,
  SRC_ONLY = 0b10,
  DST_ONLY = 0b100,
  CROSS = 0b1000,
  FULL = 0b10000
};

struct TerminatorAnalysis : public noelle::DependenceAnalysis {
  static char ID;

  using Dependence = noelle::DGEdge<llvm::Value, llvm::Value>;

  TerminatorAnalysis(noelle::Noelle &noelle);

  ~TerminatorAnalysis();

  void run();

  void run(std::set<noelle::LoopStructure *> &LSs);

  bool canThisDependenceBeLoopCarried(Dependence *LCD,
                                      noelle::LoopStructure &LS) override;

  void printDependence(const Dependence *LCD) const;

  bool isLDTCBegin(const llvm::Instruction *I) const;

  bool isLDTCEnd(const llvm::Instruction *I) const;

  bool isLDTC(const llvm::Instruction *I) const;

  std::set<llvm::Instruction *> getPragmasInLoop(noelle::LoopStructure *LS,
                                                 noelle::LoopForest *LF) const;

  std::set<noelle::LoopStructure *>
  findTargetLoops(std::set<noelle::LoopStructure *> &LSs);

  bool isUnmatched(llvm::Instruction *begin) const;

  llvm::Instruction *findUnmatchedBegin(llvm::BasicBlock *BB) const;

  llvm::Instruction *findMatchingBeginSingleBlock(llvm::Instruction *end) const;

  std::set<llvm::Instruction *>
  findMatchingBegin(llvm::Instruction *end, llvm::Instruction **beginFound);

  void resolveClauses(noelle::LoopStructure *LS);

  void resolveClauses(std::set<noelle::LoopStructure *> &targetLSs);

  std::set<Clause *> getClausesOf(noelle::LoopStructure *LS);

  void printClauses() const;

  std::set<const Clause *> canBeTerminated(Dependence *LCD) const;

  std::string coverageToString(Coverage coverage);

  Coverage getCoverage(Dependence *LCD);

  void printCoverage();

  void sanityChecks();

  std::set<noelle::LoopStructure *> getLoopStructuresWithClauses();

private:
  std::set<Clause *> clauses_;
  std::map<noelle::LoopStructure *, std::set<Clause *>> loopToClauses_;
  std::map<llvm::Instruction *, Clause *> instToClause_;
  std::map<Clause *, std::set<llvm::Instruction *>> clauseToInsts_;
  std::set<noelle::LoopStructure *> candidateLSs_;
  // std::set<noelle::LoopStructure *> targetLSs_;
  std::set<Dependence *> unknownLCDs_;
  std::map<noelle::LoopStructure *, std::set<llvm::Instruction *>>
      loopToPragmas_;
  std::set<llvm::Instruction *> matchedBegins_;

  noelle::Noelle &noelle;
};
} // namespace arcana::gino

#endif // __TERMINATOR_ANALYSIS_HPP__
