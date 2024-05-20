#pragma once

#include "noelle/core/Architecture.hpp"
#include "noelle/core/LoopEnvironmentUser.hpp"
#include "noelle/core/Noelle.hpp"
#include "llvm/Pass.h"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

class WinchesterLoopEnvironmentUser : public LoopEnvironmentUser {
public:
  WinchesterLoopEnvironmentUser(
      std::unordered_map<uint32_t, uint32_t> &singleEnvIDToIndex,
      std::unordered_map<uint32_t, uint32_t> &reducibleEnvIDToIndex,
      std::set<uint32_t> &constantVars);

  inline void setSingleEnvironmentArray(Value *singleEnvArray) {
    this->singleEnvArray = singleEnvArray;
  };

  inline void setReducibleEnvironmentArray(Value *reducibleEnvArray) {
    this->reducibleEnvArray = reducibleEnvArray;
  };

  inline Value *getLocationToStoreReductionArray() {
    return this->locationToStoreReductionArray;
  };

  inline void addLiveIn(uint32_t id) override {
    if (this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end()) {
      this->liveInIDs.insert(id);
    }
  };

  inline void addLiveOut(uint32_t id) override {
    if (this->reducibleEnvIDToIndex.find(id) !=
        this->reducibleEnvIDToIndex.end()) {
      this->liveOutIDs.insert(id);
    }
  };

  Instruction *createSingleEnvironmentVariablePointer(IRBuilder<> &builder,
                                                      uint32_t envID,
                                                      Type *type);

  void createReducableEnvPtr(IRBuilder<> &builder, uint32_t envID, Type *type,
                             uint32_t reducerCount,
                             Value *reducerIndV) override;

  Instruction *getEnvPtr(uint32_t id) override;

  iterator_range<std::set<uint32_t>::iterator>
  getEnvIDsOfConstantLiveInVars(void);

  inline void setLiveOutEnvAddress(Value *liveOutEnvBitcastInst) {
    this->liveOutEnvBitcastInst = liveOutEnvBitcastInst;
  };
  inline Value *getLiveOutEnvBitcastInst() {
    return this->liveOutEnvBitcastInst;
  };

  inline void resetReducibleEnvironmentArray(IRBuilder<> &builder) {
    assert(this->reducibleEnvIDToIndex.size() > 0);
    if (this->reducibleEnvIDToIndex.size() > 1) {
      builder.CreateStore(this->reducibleEnvArray, this->liveOutEnvBitcastInst);
    } else {
      builder.CreateStore(this->reductionArrayForSingleLiveOut,
                          this->locationToStoreReductionArray);
    }
  }

private:
  std::unordered_map<uint32_t, uint32_t> &singleEnvIDToIndex;
  std::unordered_map<uint32_t, uint32_t> &reducibleEnvIDToIndex;
  std::unordered_map<uint32_t, Instruction *> singleEnvIndexToPtr;
  std::unordered_map<uint32_t, Instruction *> reducibleEnvIndexToPtr;

  Value *singleEnvArray;
  Value *reducibleEnvArray;

  std::set<uint32_t> &constLiveInIDs;
  // if we have at least 2 live-outs, then this instruction is casted after
  // loading from the context array otherwise (only 1 live-out), this is the
  // address in the context array, which reduction array will get loaded from
  Value *liveOutEnvBitcastInst;
  // this field is only used when there's only 1 live-out variable
  Value *locationToStoreReductionArray;
  Value *reductionArrayForSingleLiveOut;
};

} // namespace arcana::gino
