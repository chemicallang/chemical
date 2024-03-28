// Copyright (c) Qinetik 2024.

#include "CharValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *CharValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt8Ty();
}

llvm::Value *CharValue::llvm_value(Codegen &gen) {
    return gen.builder->getInt8((int) value);
}

#endif