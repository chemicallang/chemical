// Copyright (c) Qinetik 2024.

#include "IntValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type *IntValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt32Ty();
}

llvm::Value *IntValue::llvm_value(Codegen &gen) {
    return gen.builder->getInt32(value);
}

#endif