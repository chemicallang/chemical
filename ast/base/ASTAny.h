// Copyright (c) Qinetik 2024.

#pragma once

#ifdef COMPILER_BUILD

class Codegen;

#include "compiler/llvmfwd.h"

#endif

class ASTAny {
public:

#ifdef COMPILER_BUILD

    /**
     * provides llvm_type if this statement declares a type
     */
    virtual llvm::Type *llvm_type(Codegen &gen);

#endif


};