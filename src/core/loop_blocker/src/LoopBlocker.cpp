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
  auto IV = LGIV->getInductionVariable();
  auto ECV = LGIV->getExitConditionValue();
  auto LGInnerPHI = LGIV->getInductionVariable()->getLoopEntryPHI();
  auto InnerLatches = LS->getLatches();

  auto F = LS->getFunction();
  auto &Context = F->getContext();
  IRBuilder<> Builder(Context);

  auto ExitBB = LGIV->getExitBlockFromHeader();
  auto InnerCond = LGIV->getHeaderCompareInstructionToComputeExitCondition();
  auto InnerHeader = LS->getHeader();

  errs() << "LoopBlocker: loopID = " << LoopID << "\n";
  errs() << "LoopBlocker: LGInnerPHI = " << *LGInnerPHI << "\n";

  auto OuterHeader = BasicBlock::Create(Context, "", F);
  auto OuterLatch = BasicBlock::Create(Context, "", F);

  LGInnerPHI->getIncomingBlock(0)->getTerminator()->replaceSuccessorWith(
      InnerHeader, OuterHeader);
  InnerHeader->getTerminator()->replaceSuccessorWith(ExitBB, OuterLatch);

  PHINode *LGOuterPHI =
      nullptr; // PHI associated to Loop-Governing IV of outer loop

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

      // Thanks to LCSSA we onl need to patch the exit BB
      for (auto &eI : *ExitBB) {
        if (auto *exitPHI = dyn_cast<PHINode>(&eI)) {
          if (InnerPHI == exitPHI->getIncomingValueForBlock(InnerHeader)) {
            exitPHI->setIncomingValueForBlock(InnerHeader, OuterPHI);
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

  Builder.SetInsertPoint(OuterHeader);
  auto OuterCmp =
      Builder.CreateICmpSLT(LGOuterPHI, Builder.getInt32(numBlocks));
  auto OuterBranch = Builder.CreateCondBr(OuterCmp, InnerHeader, ExitBB);

  Builder.SetInsertPoint(InnerHeader->getFirstNonPHI());
  auto InnerIV = Builder.CreateMul(LGInnerPHI, Builder.getInt32(numBlocks));

  errs() << *F << "\n";

  return OuterHeader;
}

} // namespace arcana::gino
