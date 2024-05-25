#include <set>

#include <llvm/IR/Instructions.h>

#include <noelle/core/Noelle.hpp>

using namespace std;

using namespace arcana::noelle;

namespace arcana::gino {
void movePHINode(PHINode *phi, BasicBlock *targetBlock) {
  // Step 1: Remove the PHINode from its current position
  phi->removeFromParent();

  // Step 2: Insert the PHINode into the target BasicBlock
  // Insert it at the beginning of the target block
  targetBlock->getInstList().insert(targetBlock->getFirstInsertionPt(), phi);

  // Step 3: Update the incoming edges
  // This step might involve complex adjustments based on your specific needs
  for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
    BasicBlock *incomingBlock = phi->getIncomingBlock(i);
    Value *incomingValue = phi->getIncomingValue(i);

    // Ensure the incoming blocks and values are still valid
    // If the control flow has changed, you may need to update these edges
    if (!targetBlock->getSinglePredecessor() ||
        incomingBlock != targetBlock->getSinglePredecessor()) {
      phi->setIncomingBlock(i, incomingBlock);
    }
  }
}

LoopStructure *blockLoop(LoopContent *LC, int numBlocks) {
  auto LS = LC->getLoopStructure();
  auto LoopID = LS->getID().value();
  auto IVM = LC->getInductionVariableManager();
  auto LGIV = IVM->getLoopGoverningInductionVariable(*LC->getLoopStructure());
  auto IV = LGIV->getInductionVariable();
  auto ECV = LGIV->getExitConditionValue();
  auto InnerPHI = LGIV->getInductionVariable()->getLoopEntryPHI();
  auto InnerLatches = LS->getLatches();

  // auto &context = LS->getFunction()->getContext();
  auto F = LS->getFunction();
  auto &context = F->getContext();
  IRBuilder<> Builder(context);

  auto ExitBB = LGIV->getExitBlockFromHeader();
  auto InnerCond = LGIV->getHeaderCompareInstructionToComputeExitCondition();
  auto InnerHeader = LS->getHeader();

  errs() << "LoopBlocker: loopID = " << LoopID << ", loopEntry PHI: = " << *ECV
         << "\n";
  errs() << "LoopBlocker: loopID = " << *InnerPHI << "\n";
  errs() << "LoopBlocker: loopID = " << LoopID << ", exit value = " << *ECV
         << "\n";

  auto OuterHeader = BasicBlock::Create(context, "", F);
  auto OuterLatch = BasicBlock::Create(context, "", F);

  InnerPHI->getIncomingBlock(0)->getTerminator()->replaceSuccessorWith(
      InnerHeader, OuterHeader);
  InnerHeader->getTerminator()->replaceSuccessorWith(ExitBB, OuterLatch);

  // Builder.SetInsertPoint(OuterHeader);
  // auto OuterPHI = Builder.CreatePHI(InnerPHI->getType(),
  // InnerPHI->getNumIncomingValues());

  // Builder.SetInsertPoint(OuterHeader);
  // OuterPHI->addIncoming(InnerPHI->getIncomingValue(0),
  // InnerPHI->getIncomingBlock(0)); OuterPHI->addIncoming(OuterIncrement,
  // OuterLatch);

  // Moving all inner PHIs but InnerPHI to the outer header
  // set<PHINode*> toMove;
  // for (auto &I : *InnerHeader) {
  //   if (auto *phi = dyn_cast<PHINode>(&I)) {
  //     assert(phi->getNumIncomingValues() == 2);
  //     // The phi created from the loop-governing IV is treated differently
  //     if (phi == InnerPHI) {
  //     } else {
  //       for (int i = 0; i < phi->getNumIncomingValues(); i++) {
  //         auto BB = phi->getIncomingBlock(i);
  //         if (InnerLatches.find(BB) != InnerLatches.end()) {
  //           // BB is a latch of the inner loop.
  //           // It should now be replaced with the latch of the outer loop
  //           phi->replaceIncomingBlockWith(BB, OuterLatch);
  //         }
  //       }
  //       toMove.insert(phi);
  //     }
  //   }
  // }

  // for (auto &phi : toMove) {
  //   phi->removeFromParent();
  //   OuterHeader->getInstList().insert(OuterHeader->getFirstInsertionPt(),
  //   phi);
  // }

  PHINode *OuterPHI = nullptr;

  Builder.SetInsertPoint(OuterHeader);
  for (auto &I : *InnerHeader) {
    if (auto *phi = dyn_cast<PHINode>(&I)) {
      // Duplicate the PHI in the OuterHeader
      auto newPhi =
          Builder.CreatePHI(phi->getType(), phi->getNumIncomingValues());
      for (int i = 0; i < phi->getNumIncomingValues(); i++) {
        newPhi->addIncoming(phi->getIncomingValue(i), phi->getIncomingBlock(i));
      }

      // Rewiring new PHI with old ones
      for (int i = 0; i < phi->getNumIncomingValues(); i++) {
        auto BB = phi->getIncomingBlock(i);
        if (InnerLatches.find(BB) != InnerLatches.end()) {
          // BB is a latch of the inner loop.
          // It should now be replaced with the latch of the outer loop
          phi->setIncomingValueForBlock(BB, newPhi);
          newPhi->setIncomingValueForBlock(BB, phi);
          newPhi->replaceIncomingBlockWith(BB, OuterLatch);
        } else {
          // BB is not related to any latch of the inner loop
          phi->setIncomingValueForBlock(BB, newPhi);
          phi->replaceIncomingBlockWith(BB, OuterHeader);
        }
      }
      if (phi == InnerPHI) {
        OuterPHI = newPhi;
      }

      // Thanks to LCSSA we onl need to patch the exit BB.
      for (auto &I : *ExitBB) {
        if (auto *PHI = dyn_cast<PHINode>(&I)) {
          if (phi == PHI->getIncomingValueForBlock(InnerHeader)) {
            PHI->setIncomingValueForBlock(InnerHeader, newPhi);
          }
        } else {
          break;
        }
      }
    }
  }

  for (auto &I : *ExitBB) {
    if (auto *PHI = dyn_cast<PHINode>(&I)) {
      PHI->replaceIncomingBlockWith(InnerHeader, OuterHeader);
    } else {
      break;
    }
  }

  Builder.SetInsertPoint(OuterLatch);
  auto OuterIncrement = Builder.CreateAdd(OuterPHI, Builder.getInt32(1));
  Builder.CreateBr(OuterHeader);

  Builder.SetInsertPoint(OuterHeader);
  auto OuterCmp = Builder.CreateICmpSLT(OuterPHI, Builder.getInt32(numBlocks));
  auto OuterBranch = Builder.CreateCondBr(OuterCmp, InnerHeader, ExitBB);

  Builder.SetInsertPoint(InnerHeader->getFirstNonPHI());
  auto InnerIV = Builder.CreateMul(InnerPHI, Builder.getInt32(numBlocks));

  assert(InnerPHI->getNumIncomingValues() == 2);

  // Rewiring the incoming basic block of the PHIs in the inner loop header.
  // Now the only predecessor is the header of the outer loop.
  // Assumption: the first incoming value represents the IV starting values.
  // for (auto &I : *InnerHeader) {
  //   if (auto *phi = dyn_cast<PHINode>(&I)) {
  //     phi->setIncomingValue(0, OuterPHI);
  //     phi->setIncomingBlock(0, OuterHeader);
  //   } else {
  //     break;
  //   }
  // }

  // Rewiring the incoming basic block of the PHIs in the exit basic block.
  // Values with blocks == inner loop header need now be associated with
  // the outer loop header
  // for (auto &I : *ExitBB) {
  //   if (auto *phi = dyn_cast<PHINode>(&I)) {
  //     auto i = phi->getBasicBlockIndex(InnerHeader);
  //     phi->setIncomingBlock(i, OuterHeader);
  //   } else {
  //     break;
  //   }
  // }

  // errs() << *InnerHeader << "\n";
  // for (auto *I : InnerPHI->users()) {
  //   errs() << "Use: " << *I << "\n";
  //   if (I == InnerIV) {
  //     continue;
  //   }
  //   for (auto &op : I->operands()) {
  //     if (op.get() == InnerPHI) {
  //       op.set(InnerIV);
  //     }
  //   }
  // }
  //
  // auto *B = LS->getSuccessorWithinLoopOfTheHeader();
  // for (auto &I : *B) {
  //   if (auto *CI = dyn_cast<CallInst>(&I)) {
  //     for (auto &op : CI->operands()) {
  //       errs() << *op << "\n";
  //     }
  //   }
  // }
  //
  errs() << *F << "\n";

  return nullptr;
}

} // namespace arcana::gino
