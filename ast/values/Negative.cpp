// Copyright (c) Qinetik 2024.

#include "Negative.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value* NegativeValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNeg(value->llvm_value(gen));
}

#endif