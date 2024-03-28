// Copyright (c) Qinetik 2024.

#include "Int32Type.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *Int32Type::llvm_type(Codegen &gen) const {
    return gen.builder->getInt32Ty();
}

#endif