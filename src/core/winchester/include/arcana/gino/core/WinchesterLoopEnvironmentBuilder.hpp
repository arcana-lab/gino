#pragma once

#include "WinchesterLoopEnvironmentUser.hpp"
#include "noelle/core/Architecture.hpp"
#include "noelle/core/LoopEnvironment.hpp"
#include "noelle/core/LoopEnvironmentBuilder.hpp"
#include "noelle/core/LoopEnvironmentUser.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/Pass.h"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

class WinchesterLoopEnvironmentBuilder : public LoopEnvironmentBuilder {
public:
  WinchesterLoopEnvironmentBuilder(
      LLVMContext &cxt, LoopEnvironment *env,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          shouldThisVariableBeReduced,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          shouldThisVariableBeSkipped,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          isConstantLiveInVariable,
      uint64_t reducerCount, uint64_t numberOfUsers, uint64_t numLevels);

  inline uint32_t getSingleEnvironmentSize() {
    return this->singleEnvIDToIndex.size();
  };

  inline uint32_t getReducibleEnvironmentSize() {
    return this->reducibleEnvIDToIndex.size();
  };

  inline ArrayType *getSingleEnvironmentArrayType() {
    return this->singleEnvArrayType;
  };

  inline ArrayType *getReducibleEnvironmentArrayType() {
    return this->reducibleEnvArrayType;
  };

  inline ArrayType *getContextArrayType() { return this->contextArrayType; };

  bool hasVariableBeenReduced(uint32_t id) const override;

  void allocateNextLevelReducibleEnvironmentArray(IRBuilder<> &builder);

  void generateNextLevelReducibleEnvironmentVariables(IRBuilder<> &builder);

  inline Value *getNextLevelEnvironmentArrayVoidPtr() {
    assert(this->reducibleEnvArrayInt8PtrNextLevel != nullptr &&
           "Next level envrionment array void ptr hasn't been set yet\n");
    return this->reducibleEnvArrayInt8PtrNextLevel;
  };

  BasicBlock *reduceLiveOutVariablesInTask(
      int taskIndex, BasicBlock *loopHandlerBB, IRBuilder<> &loopHandlerBuilder,
      const std::unordered_map<uint32_t, Instruction::BinaryOps>
          &reducibleBinaryOps,
      const std::unordered_map<uint32_t, Value *> &initialValues,
      Instruction *cmpInst, BasicBlock *bottomHalfBB, Value *numOfReducerV);

  void allocateSingleEnvironmentArray(IRBuilder<> &builder);

  void allocateReducibleEnvironmentArray(IRBuilder<> &builder);

  void generateEnvVariables(IRBuilder<> &builder, uint32_t reducerCount);

  inline bool isIncludedEnvironmentVariable(uint32_t id) const override {
    return (this->singleEnvIDToIndex.find(id) !=
            this->singleEnvIDToIndex.end()) ||
           (this->reducibleEnvIDToIndex.find(id) !=
            this->reducibleEnvIDToIndex.end());
  }

  Value *getEnvironmentVariable(uint32_t id) const override;

  BasicBlock *reduceLiveOutVariablesWithInitialValue(
      BasicBlock *bb, IRBuilder<> &builder,
      const std::unordered_map<uint32_t, Instruction::BinaryOps>
          &reducibleBinaryOps,
      const std::unordered_map<uint32_t, Value *> &initialValues);

  Value *getAccumulatedReducedEnvironmentVariable(uint32_t id) const override;

  inline AllocaInst *getLiveOutReductionArray(uint32_t id) {
    return this->envIndexToVectorOfReducableVarNextLevel
        [this->reducibleEnvIDToIndex[id]];
  };

  bool isConstantLiveIn(uint32_t id) {
    return this->constantVars.find(id) != this->constantVars.end();
  }

  inline uint32_t getIndexOLiveIn(uint32_t id) {
    assert(this->singleEnvIDToIndex.find(id) !=
               this->singleEnvIDToIndex.end() &&
           "liveIn variable is not included in the builder");
    return this->singleEnvIDToIndex[id];
  }

  inline uint32_t getIndexOfLiveOut(uint32_t id) {
    assert(this->reducibleEnvIDToIndex.find(id) !=
               this->reducibleEnvIDToIndex.end() &&
           "liveOut variable is not included in the builder");
    return this->reducibleEnvIDToIndex[id];
  }

  inline Value *getSingleEnvironmentArray() { return this->singleEnvArray; };

  inline Value *getReducibleEnvironmentArray() {
    return this->reducibleEnvArray;
  };

  inline Value *
  getReducibleVariableOfIndexGivenEnvIndex(uint32_t index,
                                           uint64_t reducer_index) {
    assert(this->envIndexToReducableVar.find(index) !=
               this->envIndexToReducableVar.end() &&
           "no reducible variable found at the given index");
    return this->envIndexToReducableVar[index][reducer_index];
  }

  inline Value *getLocationOfSingleVariable(uint32_t id) {
    return this->envIndexToVar[this->getIndexOLiveIn(id)];
  }

  inline Value *getOnlyReductionArray() { return this->onlyReductionArray; }

private:
  void initializeBuilder(const std::vector<Type *> &varTypes,
                         const std::set<uint32_t> &singleVarIDs,
                         const std::set<uint32_t> &reducibleVarIDs,
                         uint64_t reducerCount, uint64_t numberOfUsers);
  void createUsers(uint32_t numUsers);

  std::unordered_map<uint32_t, uint32_t> singleEnvIDToIndex;
  std::unordered_map<uint32_t, uint32_t> reducibleEnvIDToIndex;
  std::unordered_map<uint32_t, uint32_t> indexToSingleEnvID;
  std::unordered_map<uint32_t, uint32_t> indexToReducibleEnvID;

  Value *singleEnvArray;
  ArrayType *singleEnvArrayType;
  Value *reducibleEnvArray;
  Value *reducibleEnvArrayNextLevel;
  Value *reducibleEnvArrayInt8PtrNextLevel;
  ArrayType *reducibleEnvArrayType;
  std::unordered_map<uint32_t, AllocaInst *>
      envIndexToVectorOfReducableVarNextLevel; // envIndex to reducible
                                               // envrionment variable array
  std::unordered_map<uint32_t, Value *>
      envIndexToAccumulatedReducableVarForTask; // envIndex to accumulated of
                                                // reduction result

  std::vector<Type *> singleEnvTypes;
  std::vector<Type *> reducibleEnvTypes;

  uint64_t numLevels;
  uint64_t valuesInCacheLine;
  ArrayType *contextArrayType;
  std::set<uint32_t> constantVars;

  // These two fields are used only when there's 1 live-out variable
  Value *onlyReductionArray;
};

} // namespace arcana::gino
