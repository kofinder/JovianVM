#include "jovianvm.h"

llvm::LLVMContext theContext;

llvm::IRBuilder<> Builder(theContext);

std::unique_ptr<llvm::Module> theModule;

std::map<std::string, llvm::Value *> namedValues;