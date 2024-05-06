// Copyright (c) Qinetik 2024.

#include "DoubleValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type *DoubleValue::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Value *DoubleValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), value);
}

#endif