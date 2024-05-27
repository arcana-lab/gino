#include <set>

#include <llvm/IR/Instructions.h>

#include <noelle/core/Noelle.hpp>

using namespace std;

using namespace arcana::noelle;

namespace arcana::gino {

BasicBlock *blockLoop(LoopContent *LC, int numBlocks) {
  auto LS = LC->getLoopStructure();
  auto LoopID = LS->getID().value();
  auto IVM = LC->getInductionVariableManager();
  auto LGIV = IVM->getLoopGoverningInductionVariable(*LC->getLoopStructure());
  auto ECV = LGIV->getExitConditionValue();
  auto LGInnerPHI = LGIV->getInductionVariable()->getLoopEntryPHI();
  auto InnerLatches = LS->getLatches();

  auto F = LS->getFunction();
  auto &Context = F->getContext();
  IRBuilder<> Builder(Context);

  auto ExitBB = LGIV->getExitBlockFromHeader();
  auto InnerCond = LGIV->getHeaderCompareInstructionToComputeExitCondition();
  auto InnerHeader = LS->getHeader();

  // errs() << "LoopBlocker: LGInnerPHI = " << *LGInnerPHI << "\n";

  auto OuterHeader = BasicBlock::Create(Context, "", F);

  Value *InnerOriginalStartIdx = nullptr;

  // All predecessors of the original header (InnerHeader) must
  // now branch to the new header
  for (int i = 0; i < LGInnerPHI->getNumIncomingValues(); i++) {
    auto BB = LGInnerPHI->getIncomingBlock(i);
    if (InnerLatches.find(BB) == InnerLatches.end()) {
      // BB is not a latch of the loop.
      // It needs to be rewired to the new header
      BB->getTerminator()->replaceSuccessorWith(InnerHeader, OuterHeader);

      // The induction variable must have only one initial value
      assert(InnerOriginalStartIdx == nullptr);
      InnerOriginalStartIdx = LGInnerPHI->getIncomingValue(i);
    }
  }

  auto OuterLatch = BasicBlock::Create(Context, "", F);
  InnerHeader->getTerminator()->replaceSuccessorWith(ExitBB, OuterLatch);

  // PHI associated to Loop-Governing IV of outer loop
  PHINode *LGOuterPHI = nullptr;

  Builder.SetInsertPoint(OuterHeader);
  for (auto &I : *InnerHeader) {
    if (auto *InnerPHI = dyn_cast<PHINode>(&I)) {
      // Duplicate the InnerPHI in the OuterHeader
      auto OuterPHI = Builder.CreatePHI(InnerPHI->getType(),
                                        InnerPHI->getNumIncomingValues());
      for (int i = 0; i < InnerPHI->getNumIncomingValues(); i++) {
        OuterPHI->addIncoming(InnerPHI->getIncomingValue(i),
                              InnerPHI->getIncomingBlock(i));
      }

      // Rewiring new InnerPHI with old ones
      for (int i = 0; i < InnerPHI->getNumIncomingValues(); i++) {
        auto BB = InnerPHI->getIncomingBlock(i);
        if (InnerLatches.find(BB) != InnerLatches.end()) {
          // BB is a latch of the inner loop.
          // It should now be replaced with the latch of the outer loop
          OuterPHI->setIncomingValueForBlock(BB, InnerPHI);
          OuterPHI->replaceIncomingBlockWith(BB, OuterLatch);
        } else {
          // BB is not related to any latch of the inner loop
          InnerPHI->setIncomingValueForBlock(BB, OuterPHI);
          InnerPHI->replaceIncomingBlockWith(BB, OuterHeader);
        }
      }
      if (InnerPHI == LGInnerPHI) {
        LGOuterPHI = OuterPHI;
      }

      // Thanks to LCSSA we only need to patch the exit BB
      for (auto &I : *ExitBB) {
        if (auto *ExitPHI = dyn_cast<PHINode>(&I)) {
          if (InnerPHI == ExitPHI->getIncomingValueForBlock(InnerHeader)) {
            ExitPHI->setIncomingValueForBlock(InnerHeader, OuterPHI);
          }
        } else {
          break;
        }
      }
    }
  }

  // Adjusting PHIs in the exit block
  for (auto &I : *ExitBB) {
    if (auto *InnerPHI = dyn_cast<PHINode>(&I)) {
      InnerPHI->replaceIncomingBlockWith(InnerHeader, OuterHeader);
    } else {
      break;
    }
  }

  Builder.SetInsertPoint(OuterLatch);
  auto OuterIncrement = Builder.CreateAdd(LGOuterPHI, Builder.getInt32(1));
  Builder.CreateBr(OuterHeader);
  LGOuterPHI->setIncomingValueForBlock(OuterLatch, OuterIncrement);
  LGOuterPHI->setIncomingValue(0, Builder.getInt32(0));

  auto InnerCmp = LGIV->getHeaderCompareInstructionToComputeExitCondition();

  Builder.SetInsertPoint(OuterHeader);

  // N - InnerOriginalStartIdx
  auto NumIterations =
      Builder.CreateSub(InnerCmp->getOperand(1), InnerOriginalStartIdx);

  // InnerNewStartIdx = i * (N - i_start) / numBlocks + i_start
  auto InnerNewStartIdx = Builder.CreateAdd(
      Builder.CreateSDiv(Builder.CreateMul(LGOuterPHI, NumIterations),
                         Builder.getInt32(numBlocks)),
      InnerOriginalStartIdx);

  // InnerNewEndIdx = (i + 1) * (N - i_start) / numBlocks + i_start
  auto InnerNewEndIdx = Builder.CreateAdd(
      Builder.CreateSDiv(
          Builder.CreateMul(Builder.CreateAdd(LGOuterPHI, Builder.getInt32(1)),
                            NumIterations),
          Builder.getInt32(numBlocks)),
      InnerOriginalStartIdx);

  // for (...; j < InnerNewEndIdx; ...)
  LGInnerPHI->setIncomingValueForBlock(OuterHeader, InnerNewStartIdx);
  InnerCmp->setOperand(1, InnerNewEndIdx);

  Builder.SetInsertPoint(OuterHeader);
  auto OuterCmp =
      Builder.CreateICmpSLT(LGOuterPHI, Builder.getInt32(numBlocks));
  auto OuterBranch = Builder.CreateCondBr(OuterCmp, InnerHeader, ExitBB);

  // errs() << *F << "\n";

  return OuterHeader;
}

} // namespace arcana::gino
