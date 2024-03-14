#include "Clause.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Operator.h"

using namespace std;
using namespace llvm;

namespace arcana::gino {

Clause::Clause(Instruction *begin, Instruction *end)
    : begin_(begin), end_(end) {
  auto CI = cast<CallInst>(begin);
  variable_ = CI->getArgOperand(0);
  auto f = CI->getArgOperand(1);
  if (auto fp = dyn_cast<Function>(f)) {
    function_ = fp;
    return;
  }

  // It may happen that the clause function is stored
  // as a `i64 ptrtoint (...)`

  auto BB = begin->getParent();
  auto load = cast<LoadInst>(f);
  auto gep = cast<GetElementPtrInst>(load->getPointerOperand());
  auto alloca = gep->getPointerOperand();

  for (auto it = BasicBlock::reverse_iterator(begin); it != BB->rend(); it++) {
    if (auto store = dyn_cast<StoreInst>(&*it)) {
      if (store->getPointerOperand() == alloca) {
        auto ptr = cast<User>(store->getValueOperand())->getOperand(0);
        auto fPtr = cast<PtrToIntOperator>(ptr)->getPointerOperand();
        function_ = cast<Function>(fPtr);
        return;
      }
    }
  }
}

void Clause::print() const {
  errs() << "Terminator: Analysis: Clause: Function: " << function_->getName()
         << "\n";
}

Value *Clause::getVariable() const { return variable_; }

Function *Clause::getFunction() const { return function_; }

Instruction *Clause::getBegin() const { return begin_; }

Instruction *Clause::getEnd() const { return end_; }

} // namespace arcana::gino
