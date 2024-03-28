// Copyright (c) Qinetik 2024.

#include "BoolValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *BoolValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt1Ty();
}

llvm::Value *BoolValue::llvm_value(Codegen &gen) {
    return gen.builder->getInt1(value);
}

#endif