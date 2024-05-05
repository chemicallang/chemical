// Copyright (c) Qinetik 2024.

#include "FloatType.h"
#include "ast/values/FloatValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *FloatType::llvm_type(Codegen &gen) const {
    return gen.builder->getFloatTy();
}

#endif