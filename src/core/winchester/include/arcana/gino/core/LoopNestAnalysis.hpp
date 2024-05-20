#pragma once

#include "noelle/core/Noelle.hpp"
#include "llvm/Pass.h"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

enum class LNAVerbosity { Disabled, Enabled };

typedef struct {
  uint64_t nestID;
  uint64_t level;
  uint64_t index;
} LoopID;

class LoopNestAnalysis {
public:
  LoopNestAnalysis(StringRef functionPrefix, Noelle *noelle,
                   std::set<LoopContent *> heartbeatLoops);

  inline uint64_t getNumLoopNests() { return this->nestID; };

  uint64_t getLoopNestNumLevels(uint64_t nestID);

  std::set<LoopContent *> getLoopsAtLevel(uint64_t nestID, uint64_t level);

  inline uint64_t getLoopNestID(LoopContent *lc) {
    return this->loopToLoopID[lc].nestID;
  };
  inline uint64_t getLoopLevel(LoopContent *lc) {
    return this->loopToLoopID[lc].level;
  };
  inline uint64_t getLoopIndex(LoopContent *lc) {
    return this->loopToLoopID[lc].index;
  };

  inline bool isLeafLoop(LoopContent *lc) {
    return this->getLoopLevel(lc) + 1 ==
           this->getLoopNestNumLevels(this->getLoopNestID(lc));
  };

  std::string getLoopIDString(LoopContent *lc);

private:
  LNAVerbosity verbose;
  StringRef outputPrefix;
  StringRef functionPrefix;
  Noelle *noelle;
  std::set<LoopContent *> heartbeatLoops;

  arcana::noelle::CallGraph *callGraph;
  std::unordered_map<LoopContent *, CallGraphFunctionNode *>
      loopToCallGraphNode;
  std::unordered_map<CallGraphFunctionNode *, LoopContent *>
      callGraphNodeToLoop;
  uint64_t nestID = 0;
  std::unordered_map<LoopContent *, uint64_t> rootLoopToNestID;
  std::unordered_map<uint64_t, LoopContent *> nestIDToRootLoop;
  std::unordered_map<uint64_t, std::set<LoopContent *>> nestIDToLoops;
  std::unordered_map<LoopContent *, LoopID> loopToLoopID;

  void performLoopNestAnalysis();
  void buildLoopToCallGraphNodeMapping();
  void collectRootLoops();
  void assignLoopIDs();
  void assignLoopIDsCalledByLoop(LoopContent *callerLC, LoopID &callerLID);
};

} // namespace arcana::gino
