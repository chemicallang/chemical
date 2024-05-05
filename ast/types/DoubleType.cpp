// Copyright (c) Qinetik 2024.

#include "DoubleType.h"
#include "ast/values/DoubleValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *DoubleType::llvm_type(Codegen &gen) const {
    return gen.builder->getDoubleTy();
}

#endif