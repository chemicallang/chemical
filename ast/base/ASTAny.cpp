// Copyright (c) Qinetik 2024.

#include "ASTAny.h"
#include <iostream>

#ifdef COMPILER_BUILD

llvm::Type *ASTAny::llvm_type(Codegen &gen) {
    throw std::runtime_error("llvm_type called on ASTAny");
}

#endif