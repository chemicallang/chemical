// Copyright (c) Qinetik 2024.

#include "NotValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *NotValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateNot(value->llvm_value(gen));
}

#endif