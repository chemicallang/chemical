// Copyright (c) Qinetik 2024.

#include "AnyType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *AnyType::llvm_type(Codegen &gen) const {
    throw std::runtime_error("llvm_type called on any type");
}

#endif