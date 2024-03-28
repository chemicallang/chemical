// Copyright (c) Qinetik 2024.

#include "StringType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *StringType::llvm_type(Codegen &gen) const {
    return gen.builder->getInt8PtrTy();
}

#endif