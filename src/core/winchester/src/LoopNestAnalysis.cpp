#include "arcana/gino/core/LoopNestAnalysis.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

static cl::opt<int> LNAVerbose("heartbeat-loop-nest-analysis-verbose",
                               cl::ZeroOrMore, cl::Hidden,
                               cl::desc("Winchester Loop Nest Analysis verbose "
                                        "output (0: disabled, 1: enabled)"));

LoopNestAnalysis::LoopNestAnalysis(StringRef functionPrefix, Noelle *noelle,
                                   std::set<LoopContent *> heartbeatLoops)
    : functionPrefix(functionPrefix),
      outputPrefix("Winchester Loop Nest Analysis: "), noelle(noelle),
      heartbeatLoops(heartbeatLoops) {
  /*
   * Fetch command line options.
   */
  this->verbose = static_cast<LNAVerbosity>(LNAVerbose.getValue());

  if (this->verbose > LNAVerbosity::Disabled) {
    errs() << this->outputPrefix << "Start\n";
  }

  /*
   * Compute the function call graph.
   */
  auto fm = noelle->getFunctionsManager();
  this->callGraph = fm->getProgramCallGraph();
  assert(this->callGraph != nullptr && "Program call graph not found");

  this->performLoopNestAnalysis();

  if (this->verbose > LNAVerbosity::Disabled) {
    errs() << this->outputPrefix << "End\n";
  }

  return;
}

void LoopNestAnalysis::performLoopNestAnalysis() {
  /*
   * Build one-to-one mapping between heartbeat loop and call graph node.
   */
  this->buildLoopToCallGraphNodeMapping();

  /*
   * Collect all root loops and assign nestID.
   */
  this->collectRootLoops();

  /*
   * Assign loop ID to all heartbeat loops.
   */
  this->assignLoopIDs();

  return;
}

void LoopNestAnalysis::buildLoopToCallGraphNodeMapping() {
  for (auto heartbeatLoop : this->heartbeatLoops) {
    auto ls = heartbeatLoop->getLoopStructure();
    auto fn = ls->getFunction();
    auto callGraphNode = this->callGraph->getFunctionNode(fn);
    assert(callGraphNode != nullptr && "Call graph node not found");

    this->loopToCallGraphNode[heartbeatLoop] = callGraphNode;
    this->callGraphNodeToLoop[callGraphNode] = heartbeatLoop;
  }

  return;
}

void LoopNestAnalysis::collectRootLoops() {
  for (auto loopToCallGraphNodePair : this->loopToCallGraphNode) {
    auto lc = loopToCallGraphNodePair.first;
    auto calleeNode = loopToCallGraphNodePair.second;
    auto callerEdges = this->callGraph->getIncomingEdges(calleeNode);
    assert(callerEdges.size() == 1 &&
           "Outlined loop is called by multiple functions");
    auto callerNode = (*callerEdges.begin())->getCaller();
    auto callerFunction = callerNode->getFunction();

    /*
     * A loop called by an outlined heartbeat loop is not a root loop.
     */
    if (callerFunction->getName().contains(this->functionPrefix)) {
      continue;
    }

    this->rootLoopToNestID[lc] = this->nestID;
    this->nestIDToRootLoop[this->nestID] = lc;
    this->nestIDToLoops[this->nestID] = std::set<LoopContent *>{lc};
    this->loopToLoopID[lc] = LoopID{this->nestID, 0, 0};
    this->nestID++;
  }

  if (this->verbose > LNAVerbosity::Disabled) {
    errs() << this->outputPrefix << "There are "
           << this->rootLoopToNestID.size() << " loop nests\n";
    for (uint64_t i = 0; i < this->nestID; i++) {
      errs() << this->outputPrefix << "  nestID " << i << "\n";
      auto rootLC = this->nestIDToRootLoop[i];
      auto rootLS = rootLC->getLoopStructure();
      errs() << this->outputPrefix << "    Function \""
             << rootLS->getFunction()->getName() << "\"\n";
      errs() << this->outputPrefix << "      Loop entry instruction \""
             << *rootLS->getEntryInstruction() << "\"\n";
    }
  }

  return;
}

void LoopNestAnalysis::assignLoopIDs() {
  for (auto nestIDToRootLoopPair : this->nestIDToRootLoop) {
    auto rootLC = nestIDToRootLoopPair.second;
    auto rootLID = this->loopToLoopID[rootLC];

    /*
     * Assign loop IDs called by the root loop.
     */
    assignLoopIDsCalledByLoop(rootLC, rootLID);
  }

  assert(this->heartbeatLoops.size() == this->loopToLoopID.size() &&
         "Missing assigning loop IDs.");

  if (this->verbose > LNAVerbosity::Disabled) {
    errs() << this->outputPrefix << "Assign " << this->loopToLoopID.size()
           << " loops with IDs\n";
    for (uint64_t i = 0; i < this->nestID; i++) {
      errs() << this->outputPrefix << "  nestID " << i << "\n";
      for (auto lc : this->nestIDToLoops[i]) {
        auto ls = lc->getLoopStructure();
        errs() << this->outputPrefix << "    Function \""
               << ls->getFunction()->getName() << "\"\n";
        errs() << this->outputPrefix << "      Loop entry instruction \""
               << *ls->getEntryInstruction() << "\"\n";
        auto level = this->loopToLoopID[lc].level;
        auto index = this->loopToLoopID[lc].index;
        errs() << this->outputPrefix << "        Loop ID (nestID = " << i
               << ", level = " << level << ", index = " << index << ")\n";
      }
    }
  }

  return;
}

void LoopNestAnalysis::assignLoopIDsCalledByLoop(LoopContent *callerLC,
                                                 LoopID &callerLID) {
  auto callerNode = this->loopToCallGraphNode[callerLC];
  auto calleeEdges = this->callGraph->getOutgoingEdges(callerNode);

  // TODO: use dominator relationship to determine loop index.
  uint64_t index = 0;
  for (auto calleeEdge : calleeEdges) {
    auto calleeNode = calleeEdge->getCallee();
    auto calleeLC = this->callGraphNodeToLoop[calleeNode];

    /*
     * Construct callee loop ID.
     */
    auto nestID = callerLID.nestID;
    auto callerLevel = callerLID.level;
    LoopID calleeLID = {nestID, callerLevel + 1, index};
    this->nestIDToLoops[nestID].insert(calleeLC);
    this->loopToLoopID[calleeLC] = calleeLID;

    /*
     * Assign loop IDs recursively through DFS.
     */
    assignLoopIDsCalledByLoop(calleeLC, calleeLID);
    index++;
  }

  return;
}

uint64_t LoopNestAnalysis::getLoopNestNumLevels(uint64_t nestID) {
  uint64_t numLevels = 0;

  for (auto lc : this->nestIDToLoops[nestID]) {
    auto loopID = this->loopToLoopID[lc];
    auto level = loopID.level;
    if (level > numLevels) {
      numLevels = level;
    }
  }

  return numLevels + 1;
}

std::set<LoopContent *> LoopNestAnalysis::getLoopsAtLevel(uint64_t nestID,
                                                          uint64_t level) {
  std::set<LoopContent *> loops;

  for (auto lc : this->nestIDToLoops[nestID]) {
    auto loopID = this->loopToLoopID[lc];
    if (loopID.level == level) {
      loops.insert(lc);
    }
  }

  return loops;
}

std::string LoopNestAnalysis::getLoopIDString(LoopContent *lc) {
  auto loopID = this->loopToLoopID[lc];
  auto loopIDString = std::string("nest_")
                          .append(std::to_string(loopID.nestID))
                          .append("_level_")
                          .append(std::to_string(loopID.level))
                          .append("_index_")
                          .append(std::to_string(loopID.index));

  return loopIDString;
}

} // namespace arcana::gino
