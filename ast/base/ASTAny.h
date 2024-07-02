// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>

class Value;

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

    /**
     * chain type provides llvm based on access chain, for example
     * unions don't consider the largest member type, instead the one that has been accessed
     */
    virtual llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<Value>>& values, unsigned index) {
        return llvm_type(gen);
    }

#endif


};