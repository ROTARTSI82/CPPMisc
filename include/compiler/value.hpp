#pragma once

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

#include "compiler/type.hpp"

#include <memory>

// Wrapper class around llvm::Value
class Value {
public:
    llvm::Value *llvm;
    Type *type;

    Value(llvm::Value *llvm, Type *type) : llvm(llvm), type(type) {}
};
