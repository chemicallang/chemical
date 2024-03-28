// Copyright (c) Qinetik 2024.

#include "BoolType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *BoolType::llvm_type(Codegen &gen) const {
    return gen.builder->getInt1Ty();
}

#endif