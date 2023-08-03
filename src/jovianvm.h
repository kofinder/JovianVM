#ifndef __JOVIANVM_H__
#define __JOVIANVM_H__

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <map>

extern llvm::LLVMContext theContext;

extern llvm::IRBuilder<> builder;

extern std::unique_ptr<llvm::Module> theModule;

extern std::map<std::string, llvm::Value *> namedValues;

#endif
