// Copyright (c) Qinetik 2024.

#include "VoidType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *VoidType::llvm_type(Codegen &gen) const {
    return gen.builder->getVoidTy();
}

#endif