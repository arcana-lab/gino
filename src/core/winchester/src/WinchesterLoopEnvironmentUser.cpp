#include "arcana/gino/core/WinchesterLoopEnvironmentUser.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

WinchesterLoopEnvironmentUser::WinchesterLoopEnvironmentUser(
    std::unordered_map<uint32_t, uint32_t> &singleEnvIDToIndex,
    std::unordered_map<uint32_t, uint32_t> &reducibleEnvIDToIndex,
    std::set<uint32_t> &constLiveInIDs)
    : LoopEnvironmentUser{*(new std::unordered_map<uint32_t, uint32_t>())},
      singleEnvIDToIndex{singleEnvIDToIndex},
      reducibleEnvIDToIndex{reducibleEnvIDToIndex},
      constLiveInIDs{constLiveInIDs} {
  singleEnvIndexToPtr.clear();
  reducibleEnvIndexToPtr.clear();

  return;
}

Instruction *
WinchesterLoopEnvironmentUser::createSingleEnvironmentVariablePointer(
    IRBuilder<> &builder, uint32_t envID, Type *type) {
  if (!this->singleEnvArray) {
    errs() << "A bitcast inst to the single environment array has not been set "
              "for this user!\n";
  }

  assert(this->singleEnvIDToIndex.find(envID) !=
             this->singleEnvIDToIndex.end() &&
         "The single environment variable is not included in the user\n");
  auto singleEnvIndex = this->singleEnvIDToIndex[envID];

  auto int64 = IntegerType::get(builder.getContext(), 64);
  auto zeroV = cast<Value>(ConstantInt::get(int64, 0));

  auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);
  auto envIndV =
      cast<Value>(ConstantInt::get(int64, singleEnvIndex * valuesInCacheLine));
  auto envGEP = builder.CreateInBoundsGEP(
      this->singleEnvArray->getType()->getPointerElementType(),
      this->singleEnvArray, ArrayRef<Value *>({zeroV, envIndV}));
  auto envPtr = builder.CreateBitCast(envGEP, PointerType::getUnqual(type));

  auto ptrInst = cast<Instruction>(envPtr);
  this->singleEnvIndexToPtr[singleEnvIndex] = ptrInst;

  return ptrInst;
}

void WinchesterLoopEnvironmentUser::createReducableEnvPtr(IRBuilder<> &builder,
                                                          uint32_t envID,
                                                          Type *type,
                                                          uint32_t reducerCount,
                                                          Value *reducerIndV) {
  if (this->reducibleEnvIDToIndex.size() > 1 && !this->reducibleEnvArray) {
    errs() << "A reference to the environment array has not been set for this "
              "user!\n";
    abort();
  }

  if (this->reducibleEnvIDToIndex.size() > 1) {
    errs() << "reducible environment array (array that stores the reduction "
              "array for all live-outs): "
           << *reducibleEnvArray << "\n";
  }

  assert(this->reducibleEnvIDToIndex.find(envID) !=
             this->reducibleEnvIDToIndex.end() &&
         "The reducible environment variable is not included in the user\n");
  auto envIndex = this->reducibleEnvIDToIndex[envID];
  errs() << "live-out index in the live-out environment array: " << envIndex
         << "\n";

  auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);
  auto int64 = IntegerType::get(builder.getContext(), 64);
  auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
  auto envIndV =
      cast<Value>(ConstantInt::get(int64, envIndex * valuesInCacheLine));
  Value *envReduceGEP;
  if (this->reducibleEnvIDToIndex.size() > 1) {
    // we have at least 2 live-outs, loaded from the reducible env array
    envReduceGEP = builder.CreateInBoundsGEP(
        this->reducibleEnvArray->getType()->getPointerElementType(),
        this->reducibleEnvArray, ArrayRef<Value *>({zeroV, envIndV}),
        std::string("reductionArrayLiveOut_")
            .append(std::to_string(envIndex))
            .append("_addr"));
  } else {
    // load from the context array directly
    envReduceGEP = this->getLiveOutEnvBitcastInst();
  }

  auto envReducePtr = builder.CreateBitCast(
      envReduceGEP,
      PointerType::getUnqual(PointerType::getUnqual(
          ArrayType::get(type, reducerCount * valuesInCacheLine))),
      std::string("reductionArrayLiveOut_")
          .append(std::to_string(envIndex))
          .append("_addr_correctly_casted"));
  if (this->reducibleEnvIDToIndex.size() == 1) {
    // 1 live-out, cache this location so reduction array for next live-out can
    // store to this location directly
    this->locationToStoreReductionArray = envReducePtr;
  }
  auto reducibleArrayPtr = builder.CreateLoad(
      envReducePtr->getType()->getPointerElementType(), envReducePtr,
      std::string("reductionArrayLiveOut_").append(std::to_string(envIndex)));
  if (this->reducibleEnvIDToIndex.size() == 1) {
    // 1 live-out, cache this reduction array so we can restore it directly
    this->reductionArrayForSingleLiveOut = reducibleArrayPtr;
  }

  auto reduceIndAlignedV = builder.CreateMul(
      reducerIndV, ConstantInt::get(int64, valuesInCacheLine), "myIndex");
  auto envGEP = builder.CreateInBoundsGEP(
      reducibleArrayPtr->getType()->getPointerElementType(), reducibleArrayPtr,
      ArrayRef<Value *>({zeroV, reduceIndAlignedV}),
      std::string("liveOut_").append(std::to_string(envIndex)).append("_Ptr"));
  auto envPtr = builder.CreateBitCast(envGEP, PointerType::getUnqual(type),
                                      std::string("liveOut_")
                                          .append(std::to_string(envIndex))
                                          .append("_PtrCasted"));
  errs() << "reducible variable pointer (envPtr): " << *envPtr << "\n";

  this->reducibleEnvIndexToPtr[envIndex] = cast<Instruction>(envPtr);

  return;
}

Instruction *WinchesterLoopEnvironmentUser::getEnvPtr(uint32_t id) {

  assert((this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end() ||
          this->reducibleEnvIDToIndex.find(id) !=
              this->reducibleEnvIDToIndex.end()) &&
         "The environment variable is not included in the user\n");

  errs() << "inside WinchesterLoopEnvironmentUser::getEnvPtr\n";
  for (auto pair : this->reducibleEnvIDToIndex) {
    errs() << pair.first << ", " << pair.second << "\n";
  }

  Instruction *ptr = nullptr;
  if (this->liveInIDs.find(id) != this->liveInIDs.end()) {
    auto ind = this->singleEnvIDToIndex[id];
    ptr = this->singleEnvIndexToPtr[ind];
    assert(ptr != nullptr);
  } else {
    assert(this->liveOutIDs.find(id) != this->liveOutIDs.end() &&
           "envID is not included in both live-in and live-out in the user\n");
    auto ind = this->reducibleEnvIDToIndex[id];
    ptr = this->reducibleEnvIndexToPtr[ind];
    assert(ptr != nullptr);
  }

  return ptr;
}

iterator_range<std::set<uint32_t>::iterator>
WinchesterLoopEnvironmentUser::getEnvIDsOfConstantLiveInVars() {
  return make_range(this->constLiveInIDs.begin(), this->constLiveInIDs.end());
}

} // namespace arcana::gino
