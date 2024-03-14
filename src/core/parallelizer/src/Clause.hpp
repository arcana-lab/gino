#ifndef __CLAUSE_HPP__
#define __CLAUSE_HPP__

#include "llvm/IR/Instructions.h"

namespace arcana::gino {

class Clause {
public:
  Clause(llvm::Instruction *begin, llvm::Instruction *end);

  void print() const;

  llvm::Value *getVariable() const;
  llvm::Function *getFunction() const;
  llvm::Instruction *getBegin() const;
  llvm::Instruction *getEnd() const;

private:
  llvm::Value *variable_;
  llvm::Function *function_;
  llvm::Instruction *begin_;
  llvm::Instruction *end_;
};

} // namespace arcana::gino

#endif
