#ifndef __CLAUSE_HPP__
#define __CLAUSE_HPP__

#include <vector>

#include "llvm/IR/Instructions.h"

namespace arcana::gino {

class Clause {
public:
  Clause(llvm::Instruction *begin, llvm::Instruction *end);

  void print() const;

  llvm::Value *getVariable() const;
  llvm::Value *getDefaultValue() const;
  llvm::Function *getFunction() const;
  llvm::Instruction *getBegin() const;
  llvm::Instruction *getEnd() const;
  std::vector<llvm::Value *> getCallArguments() const;

private:
  llvm::Value *variable_;
  llvm::Value *default_;
  llvm::Function *function_;
  llvm::Instruction *begin_;
  llvm::Instruction *end_;
  std::vector<llvm::Value *> arguments_;
};

} // namespace arcana::gino

#endif
