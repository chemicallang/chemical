// Copyright (c) Qinetik 2024.

#include "StringType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "CharType.h"

llvm::Type *StringType::llvm_type(Codegen &gen) const {
    return gen.builder->getInt8PtrTy();
}

#endif

std::unique_ptr<BaseType> StringType::create_child_type() const {
    return std::unique_ptr<BaseType>(new CharType());
}