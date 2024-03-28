// Copyright (c) Qinetik 2024.

#include "CharType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *CharType::llvm_type(Codegen &gen) const {
    return gen.builder->getInt8Ty();
}

#endif