#include "arcana/gino/core/LoopSliceTask.hpp"
#include "arcana/gino/core/Winchester.hpp"
#include "arcana/gino/core/WinchesterLoopEnvironmentBuilder.hpp"
#include "arcana/gino/core/WinchesterLoopEnvironmentUser.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::gino {

void Winchester::generateLoopClosure(LoopContent *lc) {
  auto le = lc->getEnvironment();
  auto ls = lc->getLoopStructure();

  auto isReducible = [](uint32_t id, bool isLiveOut) -> bool {
    if (!isLiveOut) {
      return false;
    }
    return true;
  };
  auto shouldThisVariableBeSkipped = [&](uint32_t variableID,
                                         bool isLiveOut) -> bool {
    // if a variable is a skipped live-in, then skip it here
    if (this->loopToSkippedLiveIns[lc].find(le->getProducer(variableID)) !=
        this->loopToSkippedLiveIns[lc].end()) {
      errs() << "skip " << *(le->getProducer(variableID)) << " of id "
             << variableID << " because of the skipped liveIns\n";
      return true;
    }
    // if a variable is passed as a parameter to a callee and used only as
    // initial/exit condition of the loop, then skip it here
    auto producer = le->getProducer(variableID);
    auto IVM = lc->getInductionVariableManager();
    auto GIV = IVM->getLoopGoverningInductionVariable();
    auto IV = GIV->getInductionVariable();
    auto startValue = IV->getStartValue();
    auto exitValue = GIV->getExitConditionValue();
    if (producer->getNumUses() == 1 &&
        (producer == startValue || producer == exitValue)) {
      errs() << "skip " << *(le->getProducer(variableID)) << " of id "
             << variableID
             << " because of initial/exit condition of the callee loop\n";
      return true;
    }
    // if a variable is a constant live-in variable (loop invariant), skip it
    auto envProducer = le->getProducer(variableID);
    if (this->loopToConstantLiveIns[lc].find(envProducer) !=
        this->loopToConstantLiveIns[lc].end()) {
      errs() << "skip " << *envProducer << " of id " << variableID
             << " because of constant live-in\n";
      return true;
    }
    return false;
  };
  auto isConstantLiveInVariable = [&](uint32_t variableID,
                                      bool isLiveOut) -> bool {
    auto envProducer = le->getProducer(variableID);
    if (this->loopToConstantLiveIns[lc].find(envProducer) !=
        this->loopToConstantLiveIns[lc].end()) {
      errs() << "constant live-in " << *envProducer << " of id " << variableID
             << "\n";
      return true;
    }
    return false;
  };
  this->initializeEnvironmentBuilder(
      lc, isReducible, shouldThisVariableBeSkipped, isConstantLiveInVariable);

  /*
   * Load all loop live-in values at the entry point of the task.
   */
  auto envUser = (WinchesterLoopEnvironmentUser *)this->envBuilder->getUser(0);
  for (auto envID : le->getEnvIDsOfLiveInVars()) {
    envUser->addLiveIn(envID);
  }
  for (auto envID : le->getEnvIDsOfLiveOutVars()) {
    envUser->addLiveOut(envID);
  }
  this->generateCodeToLoadLiveInVariables(lc, 0);

  /*
   * Fix the data flow inside the loop-slice task.
   */
  this->lsTask->adjustDataAndControlFlowToUseClones();

  /*
   * Handle the reduction variables.
   */
  this->setReducableVariablesToBeginAtIdentityValue(lc, 0);

  /*
   * Add the jump from the entry of the function after loading all live-ins to
   * the header of the cloned loop.
   */
  auto headerClone =
      this->lsTask->getCloneOfOriginalBasicBlock(ls->getHeader());
  IRBuilder<> entryBuilder(this->lsTask->getEntry());
  entryBuilder.CreateBr(headerClone);

  /*
   * Add the final return to the single task's exit block.
   */
  IRBuilder<> exitB(lsTask->getExit());
  exitB.CreateRetVoid();

  /*
   * Store final results to loop live-out variables. Note this occurs after
   * all other code is generated. Propagated PHIs through the generated
   * outer loop might affect the values stored
   */
  this->generateCodeToStoreLiveOutVariables(lc, 0);

  /*
   * Allocate the reduction environment array for the next level inside task
   */
  this->allocateNextLevelReducibleEnvironmentInsideTask(lc, 0);

  errs() << this->outputPrefix
         << "loop-slice task after generating loop closure\n";
  errs() << *this->lsTask->getTaskBody() << "\n";

  return;
}

void Winchester::initializeEnvironmentBuilder(
    LoopContent *lc,
    std::function<bool(uint32_t variableID, bool isLiveOut)>
        shouldThisVariableBeReduced,
    std::function<bool(uint32_t variableID, bool isLiveOut)>
        shouldThisVariableBeSkipped,
    std::function<bool(uint32_t variableID, bool isLiveOut)>
        isConstantLiveInVariable) {

  auto environment = lc->getEnvironment();
  assert(environment != nullptr);

  if (this->tasks.size() == 0) {
    errs()
        << "ERROR: Parallelization technique tasks haven't been created yet!\n"
        << "\tTheir environment builders can't be initialized until they "
           "are.\n";
    abort();
  }

  auto program = this->noelle->getProgram();
  // envBuilder is a field in ParallelizationTechnique
  this->envBuilder = new WinchesterLoopEnvironmentBuilder(
      program->getContext(), environment, shouldThisVariableBeReduced,
      shouldThisVariableBeSkipped, isConstantLiveInVariable,
      this->numTaskInstances, this->tasks.size(),
      this->lna->getLoopNestNumLevels(this->nestID));
  this->initializeLoopEnvironmentUsers();

  return;
}

void Winchester::initializeLoopEnvironmentUsers() {
  for (auto i = 0; i < this->tasks.size(); ++i) {
    auto task = (LoopSliceTask *)this->tasks[i];
    assert(task != nullptr);
    auto envBuilder = (WinchesterLoopEnvironmentBuilder *)this->envBuilder;
    assert(envBuilder != nullptr);
    auto envUser =
        (WinchesterLoopEnvironmentUser *)this->envBuilder->getUser(i);
    assert(envUser != nullptr);

    auto entryBlock = task->getEntry();
    IRBuilder<> entryBuilder{entryBlock};

    // Create a bitcast instruction from i8* pointer to context array type
    this->LSTContextBitCastInst = entryBuilder.CreateBitCast(
        task->getLSTContextPointerArg(),
        PointerType::getUnqual(envBuilder->getContextArrayType()),
        "LSTContextArray");

    // Calculate the base index of my context
    auto int64 = IntegerType::get(entryBuilder.getContext(), 64);
    auto zeroV = entryBuilder.getInt64(0);

    // Don't load from the environment pointer if the size is
    // 0 for either single or reducible environment
    if (envBuilder->getSingleEnvironmentSize() > 0) {
      auto liveInEnvPtrGEPInst = entryBuilder.CreateInBoundsGEP(
          envBuilder->getContextArrayType(), (Value *)LSTContextBitCastInst,
          ArrayRef<Value *>({zeroV,
                             // ConstantInt::get(entryBuilder.getInt64Ty(),
                             // this->lna->getLoopLevel(this->lc) *
                             // this->valuesInCacheLine + this->liveInEnvIndex)
                             ConstantInt::get(entryBuilder.getInt64Ty(), 0)}),
          "liveInEnvPtr");
      auto liveInEnvBitcastInst = entryBuilder.CreateBitCast(
          cast<Value>(liveInEnvPtrGEPInst),
          PointerType::getUnqual(PointerType::getUnqual(
              envBuilder->getSingleEnvironmentArrayType())),
          "liveInEnvPtrCasted");
      auto liveInEnvLoadInst = entryBuilder.CreateLoad(
          PointerType::getUnqual(envBuilder->getSingleEnvironmentArrayType()),
          (Value *)liveInEnvBitcastInst, "liveInEnv");
      envUser->setSingleEnvironmentArray(liveInEnvLoadInst);
    }
    if (envBuilder->getReducibleEnvironmentSize() > 0) {
      auto liveOutEnvPtrGEPInst = entryBuilder.CreateInBoundsGEP(
          envBuilder->getContextArrayType(), (Value *)LSTContextBitCastInst,
          ArrayRef<Value *>({zeroV,
                             // ConstantInt::get(entryBuilder.getInt64Ty(),
                             // this->lna->getLoopLevel(this->lc) *
                             // this->valuesInCacheLine + this->liveOutEnvIndex)
                             ConstantInt::get(entryBuilder.getInt64Ty(),
                                              this->liveOutEnvIndex)}),
          "liveOutEnv_addr");
      if (envBuilder->getReducibleEnvironmentSize() == 1) {
        // the address to the live-out environment in the context array will be
        // used to store the reduction array
        envUser->setLiveOutEnvAddress(liveOutEnvPtrGEPInst);
      } else {
        auto liveOutEnvBitcastInst = entryBuilder.CreateBitCast(
            cast<Value>(liveOutEnvPtrGEPInst),
            PointerType::getUnqual(PointerType::getUnqual(
                envBuilder->getReducibleEnvironmentArrayType())),
            "liveOutEnv_addr_correctly_casted");
        // save this liveOutEnvBitcastInst as it will be used later by the
        // envBuilder to set the liveOutEnv for kids
        envUser->setLiveOutEnvAddress(liveOutEnvBitcastInst);
        auto liveOutEnvLoadInst = entryBuilder.CreateLoad(
            PointerType::getUnqual(
                envBuilder->getReducibleEnvironmentArrayType()),
            (Value *)liveOutEnvBitcastInst, "liveOutEnv");
        envUser->setReducibleEnvironmentArray(liveOutEnvLoadInst);
      }
    }

    // // Don't load from the environment pointer if the size is
    // // 0 for either single or reducible environment
    // if (envBuilder->getSingleEnvironmentSize() > 0) {
    //   auto singleEnvironmentBitcastInst = entryBuilder.CreateBitCast(
    //     task->getSingleEnvironment(),
    //     PointerType::getUnqual(envBuilder->getSingleEnvironmentArrayType()),
    //     "heartbeat.single_environment_variable.pointer");
    //   envUser->setSingleEnvironmentArray(singleEnvironmentBitcastInst);
    // }

    // if (envBuilder->getReducibleEnvironmentSize() > 0) {
    //   auto reducibleEnvironmentBitcastInst = entryBuilder.CreateBitCast(
    //     task->getReducibleEnvironment(),
    //     PointerType::getUnqual(envBuilder->getReducibleEnvironmentArrayType()),
    //     "heartbeat.reducible_environment_variable.pointer");
    //   envUser->setReducibleEnvironmentArray(reducibleEnvironmentBitcastInst);
    // }
  }

  return;
}

void Winchester::generateCodeToLoadLiveInVariables(LoopContent *lc,
                                                   int taskIndex) {
  auto task = this->tasks[taskIndex];
  auto env = lc->getEnvironment();
  auto envUser =
      (WinchesterLoopEnvironmentUser *)this->envBuilder->getUser(taskIndex);

  IRBuilder<> builder{task->getEntry()};

  // Load live-in variables pointer, then load from the pointer to get the
  // live-in variable
  for (auto envID : envUser->getEnvIDsOfLiveInVars()) {
    auto producer = env->getProducer(envID);
    auto envPointer = envUser->createSingleEnvironmentVariablePointer(
        builder, envID, producer->getType());
    auto metaString =
        std::string{"liveIn_variable_"}.append(std::to_string(envID));
    auto envLoad =
        builder.CreateLoad(producer->getType(), envPointer, metaString);

    task->addLiveIn(producer, envLoad);
  }

  // Cast constLiveIns to its array type
  auto constLiveIns = builder.CreateBitCast(
      this->lsTask->getInvariantsPointerArg(),
      PointerType::getUnqual(ArrayType::get(
          builder.getInt64Ty(), this->constantLiveInsArgIndexToIndex.size())),
      "constantLiveIns");

  // Load constant live-in variables
  // double *a = (double *)constLiveIns[0];
  for (const auto constEnvID : envUser->getEnvIDsOfConstantLiveInVars()) {
    auto producer = env->getProducer(constEnvID);
    errs() << "constEnvID: " << constEnvID << ", value: " << *producer << "\n";
    // get the index of constant live-in in the array
    auto arg_index = this->loopToConstantLiveIns[this->lc][producer];
    auto const_index = this->constantLiveInsArgIndexToIndex[arg_index];
    auto int64 = builder.getInt64Ty();
    auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
    auto const_index_V = cast<Value>(ConstantInt::get(int64, const_index));
    // create gep in to the constant live-in
    auto constLiveInGEPInst = builder.CreateInBoundsGEP(
        ArrayType::get(builder.getInt64Ty(),
                       this->constantLiveInsArgIndexToIndex.size()),
        cast<Value>(constLiveIns), ArrayRef<Value *>({zeroV, const_index_V}),
        std::string("constantLiveIn_")
            .append(std::to_string(const_index))
            .append("_addr"));

    // load from casted instruction
    auto constLiveInLoadInst =
        builder.CreateLoad(int64, constLiveInGEPInst,
                           std::string("constantLiveIn_")
                               .append(std::to_string(const_index))
                               .append("_casted"));

    // cast to the correct type of constant variable
    Value *constLiveInCastedInst;
    switch (producer->getType()->getTypeID()) {
    case Type::PointerTyID:
      constLiveInCastedInst = builder.CreateIntToPtr(
          constLiveInLoadInst, producer->getType(),
          std::string("constantLiveIn_").append(std::to_string(const_index)));
      break;
    case Type::IntegerTyID:
      constLiveInCastedInst = builder.CreateSExtOrTrunc(
          constLiveInLoadInst, producer->getType(),
          std::string("constantLiveIn_").append(std::to_string(const_index)));
      break;
    case Type::FloatTyID:
    case Type::DoubleTyID:
      constLiveInCastedInst = builder.CreateLoad(
          producer->getType(),
          builder.CreateIntToPtr(constLiveInLoadInst,
                                 PointerType::getUnqual(producer->getType())),
          std::string("constantLiveIn_").append(std::to_string(const_index)));
      break;
    default:
      assert(false &&
             "constLiveIn is a type other than ptr, int, float or double\n");
      exit(1);
    }

    task->addLiveIn(producer, constLiveInCastedInst);
  }

  return;
}

void Winchester::setReducableVariablesToBeginAtIdentityValue(LoopContent *lc,
                                                             int taskIndex) {
  assert(taskIndex < this->tasks.size());
  auto task = this->tasks[taskIndex];
  assert(task != nullptr);

  auto loopStructure = lc->getLoopStructure();
  auto loopPreHeader = loopStructure->getPreHeader();
  auto loopPreHeaderClone = task->getCloneOfOriginalBasicBlock(loopPreHeader);
  assert(loopPreHeaderClone != nullptr);
  auto loopHeader = loopStructure->getHeader();
  auto loopHeaderClone = task->getCloneOfOriginalBasicBlock(loopHeader);
  assert(loopHeaderClone != nullptr);

  auto environment = lc->getEnvironment();
  assert(environment != nullptr);

  /*
   * Fetch the SCCDAG.
   */
  auto sccManager = lc->getSCCManager();
  auto sccdag = sccManager->getSCCDAG();

  for (auto envID : environment->getEnvIDsOfLiveOutVars()) {
    auto isThisLiveOutVariableReducible =
        ((WinchesterLoopEnvironmentBuilder *)this->envBuilder)
            ->hasVariableBeenReduced(envID);
    if (!isThisLiveOutVariableReducible) {
      continue;
    }

    auto producer = environment->getProducer(envID);
    assert(producer != nullptr);
    assert(isa<PHINode>(producer) && "live-out producer must be a phi node\n");
    auto producerSCC = sccdag->sccOfValue(producer);
    // producerSCC->print(errs());
    auto reductionVar =
        static_cast<BinaryReductionSCC *>(sccManager->getSCCAttrs(producerSCC));
    auto loopEntryProducerPHI =
        reductionVar->getPhiThatAccumulatesValuesBetweenLoopIterations();
    assert(loopEntryProducerPHI != nullptr);

    auto producerClone = cast<PHINode>(
        task->getCloneOfOriginalInstruction(loopEntryProducerPHI));
    errs() << "phi that accumulates values between loop iterations: "
           << *producerClone << "\n";
    assert(producerClone != nullptr);

    auto incomingIndex = producerClone->getBasicBlockIndex(loopPreHeaderClone);
    assert(incomingIndex != -1 && "Doesn't find loop preheader clone as an "
                                  "entry for the reducible phi instruction\n");

    auto identityV = reductionVar->getIdentityValue();

    producerClone->setIncomingValue(incomingIndex, identityV);
  }

  return;
}

void Winchester::generateCodeToStoreLiveOutVariables(LoopContent *lc,
                                                     int taskIndex) {
  auto mm = this->noelle->getMetadataManager();
  auto env = lc->getEnvironment();
  auto task = this->tasks[taskIndex];
  assert(task != nullptr);

  // only proceed if we have a live-out environment
  if (env->getNumberOfLiveOuts() <= 0) {
    errs() << "loop doesn't have live-out environment, no need to store "
              "anything\n";
    return;
  }

  auto entryBlock = task->getEntry();
  assert(entryBlock != nullptr);
  auto entryTerminator = entryBlock->getTerminator();
  assert(entryTerminator != nullptr);
  IRBuilder<> entryBuilder{entryTerminator};

  auto &taskFunction = *task->getTaskBody();
  // auto cfgAnalysis = this->noelle->getCFGAnalysis();

  /*
   * Fetch the loop SCCDAG
   */
  auto sccManager = lc->getSCCManager();
  auto loopSCCDAG = sccManager->getSCCDAG();

  auto envUser =
      (WinchesterLoopEnvironmentUser *)this->envBuilder->getUser(taskIndex);
  for (auto envID : envUser->getEnvIDsOfLiveOutVars()) {
    auto producer = cast<Instruction>(env->getProducer(envID));
    assert(producer != nullptr);
    if (!task->doesOriginalLiveOutHaveManyClones(producer)) {
      auto singleProducerClone = task->getCloneOfOriginalInstruction(producer);
      task->addLiveOut(producer, singleProducerClone);
    }

    auto producerClones = task->getClonesOfOriginalLiveOut(producer);
    errs() << "size of producer clones: " << producerClones.size() << "\n";

    auto envType = producer->getType();
    auto isReduced = (WinchesterLoopEnvironmentBuilder *)this->envBuilder
                         ->hasVariableBeenReduced(envID);
    if (isReduced) {
      envUser->createReducableEnvPtr(
          entryBuilder, envID, envType, this->numTaskInstances,
          ((LoopSliceTask *)task)->getTaskIndexArg());
    } else {
      errs() << "All heartbeat loop live-outs must be reducible!\n";
      abort();
    }

    // Assumption from now on, all live-out variables should be reducible
    auto envPtr = envUser->getEnvPtr(envID);

    /*
     * Fetch the reduction
     */
    auto producerSCC = loopSCCDAG->sccOfValue(producer);
    auto reductionVariable =
        static_cast<BinaryReductionSCC *>(sccManager->getSCCAttrs(producerSCC));
    assert(reductionVariable != nullptr);

    // // Reducible live-out initialization
    // auto identityV = reductionVariable->getIdentityValue();
    // // OPTIMIZATION: we might not need to initialize the live-out in the
    // reduction array here
    // // can just store the final reduction result
    // auto newStore = entryBuilder.CreateStore(identityV, envPtr);
    // mm->addMetadata(newStore,
    // "heartbeat.environment_variable.live_out.reducible.initialize_private_copy",
    // std::to_string(envID));

    /*
     * Inject store instructions to propagate live-out values back to the caller
     * of the parallelized loop.
     *
     * NOTE: To support storing live outs at exit blocks and not directly where
     * the producer is executed, produce a PHI node at each store point with the
     * following incoming values: the last executed intermediate of the producer
     * that is post-dominated by that incoming block. There should only be one
     * such value assuming that store point is correctly chosen
     *
     * NOTE: This provides flexibility to parallelization schemes with modified
     * prologues or latches that have reducible live outs. Furthermore, this
     * flexibility is ONLY permitted for reducible or IV live outs as other live
     * outs can never store intermediate values of the producer.
     */
    for (auto producerClone : producerClones) {
      auto taskDS = this->noelle->getDominators(&taskFunction);

      auto insertBBs = this->determineLatestPointsToInsertLiveOutStore(
          lc, taskIndex, producerClone, isReduced, *taskDS);
      assert(insertBBs.size() == 1 &&
             "found multiple insertion point for accumulated private copy, "
             "currently doesn't handle it\n");
      for (auto BB : insertBBs) {
        auto producerValueToStore =
            this->fetchOrCreatePHIForIntermediateProducerValueOfReducibleLiveOutVariable(
                lc, taskIndex, envID, BB, *taskDS);
        this->liveOutVariableToAccumulatedPrivateCopy[envID] =
            producerValueToStore;
        // IRBuilder<> liveOutBuilder(BB);
        // auto store = liveOutBuilder.CreateStore(producerValueToStore,
        // envPtr); store->removeFromParent();

        // store->insertBefore(BB->getTerminator());
        // mm->addMetadata(store,
        // "heartbeat.environment_variable.live_out.reducible.update_private_copy",
        // std::to_string(envID));
      }

      delete taskDS;
    }
  }

  return;
}

void Winchester::allocateNextLevelReducibleEnvironmentInsideTask(
    LoopContent *lc, int taskIndex) {
  auto loopStructure = lc->getLoopStructure();
  auto loopFunction = loopStructure->getFunction();

  auto task = this->tasks[taskIndex];
  assert(task != nullptr);
  auto taskEntry = task->getEntry();
  assert(taskEntry != nullptr);
  auto entryTerminator = taskEntry->getTerminator();
  assert(entryTerminator != nullptr);

  IRBuilder<> builder{taskEntry};
  builder.SetInsertPoint(entryTerminator);
  ((WinchesterLoopEnvironmentBuilder *)this->envBuilder)
      ->allocateNextLevelReducibleEnvironmentArray(builder);
  ((WinchesterLoopEnvironmentBuilder *)this->envBuilder)
      ->generateNextLevelReducibleEnvironmentVariables(builder);

  // the pointer to the liveOutEnvironment array (if any)
  // is now pointing at the liveOutEnvironment array for kids,
  // need to reset it before returns
  // Fix: only reset the environment when there's a live-out environment for the
  // loop
  if (((WinchesterLoopEnvironmentBuilder *)this->envBuilder)
          ->getReducibleEnvironmentSize() > 0) {
    auto taskExit = task->getExit();
    auto exitReturnInst = taskExit->getTerminator();
    errs() << "terminator of the exit block of task " << *exitReturnInst
           << "\n";
    IRBuilder<> exitBuilder{taskExit};
    exitBuilder.SetInsertPoint(exitReturnInst);
    ((WinchesterLoopEnvironmentUser *)this->envBuilder->getUser(0))
        ->resetReducibleEnvironmentArray(exitBuilder);
  }

  return;
}

} // namespace arcana::gino
