// Copyright (c) Qinetik 2024.

#pragma once

#include "Visitor.h"
#include <vector>
#include <memory>

class Value;

#ifdef COMPILER_BUILD

class Codegen;

#include "compiler/llvmfwd.h"

#endif

class ChainValue;

class ASTAny {
public:

    /**
     * accept the visitor
     */
    virtual void accept(Visitor *visitor) = 0;

#ifdef COMPILER_BUILD

    /**
     * provides llvm_type if this statement declares a type
     */
    virtual llvm::Type *llvm_type(Codegen &gen);

    /**
     * chain type provides llvm based on access chain, for example
     * unions don't consider the largest member type, instead the one that has been accessed
     */
    virtual llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>>& values, unsigned index) {
        return llvm_type(gen);
    }

#endif


};