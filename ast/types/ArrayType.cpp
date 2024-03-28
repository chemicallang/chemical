// Copyright (c) Qinetik 2024.

#include "ArrayType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *ArrayType::llvm_type(Codegen &gen) const {
    return llvm::ArrayType::get(elem_type->llvm_type(gen), array_size);
}

#endif