// Copyright (c) Qinetik 2024.

#include "FloatValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type * FloatValue::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Value * FloatValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), llvm::APFloat(value));
}

#endif