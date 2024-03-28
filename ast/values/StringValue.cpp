// Copyright (c) Qinetik 2024.

#include "StringValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value * StringValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateGlobalStringPtr(value);
}

#endif