// Copyright (c) Qinetik 2024.

#pragma once

#include "Visitor.h"
#include "ASTAnyKind.h"
#include <vector>
#include <memory>
#include <string>

class Value;

#ifdef COMPILER_BUILD

class Codegen;

#include "compiler/llvmfwd.h"

#endif

class InterpretScope;

class ChainValue;

class CSTToken;

class ASTAny {
public:

    /**
     * get the ast any kind for this ast any type
     */
    virtual ASTAnyKind any_kind() = 0;

    /**
     * accept the visitor
     */
    virtual void accept(Visitor *visitor) = 0;

    /**
     * the cst token for this ast any
     */
    virtual CSTToken* cst_token() = 0;

    /**
     * Interpret the current node in the given interpret scope
     */
    virtual void interpret(InterpretScope &scope);

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

    /**
     * get the string representation for this node
     */
    std::string representation();

    // ---------------------------------------------
    // unsafe As methods
    // ---------------------------------------------

    Value* as_value_unsafe() {
        return (Value*) this;
    }

    ASTNode* as_node_unsafe() {
        return (ASTNode*) this;
    }

    BaseType* as_type_unsafe() {
        return (BaseType*) this;
    }

};