// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "Interpretable.h"
#include "llvm/IR/Value.h"
#include "compiler/Codegen.h"

/**
 * @brief Base class for all AST nodes.
 */
class ASTNode : public Interpretable {
public:

    /**
     * This would return the representation of the node
     * @return
     */
    virtual std::string representation() const = 0;

    /**
     * This function is called by the scope to undeclare anything this map put on the
     * codegen's current map, which is used to resolve nodes
     * so when the scope ends, references to this variable cannot be resolved below the scope!
     * @param gen
     */
    virtual void undeclare(Codegen& gen) {
        // nothing to undeclare
    }

    /**
     * provides llvm_type if this statement declares a type
     * @param gen
     * @return
     */
    virtual llvm::Type* llvm_type(Codegen& gen) {
        throw std::runtime_error("llvm_type called on bare ASTNode");
    };

    /**
     * code_gen function that generates llvm Value
     * @return
     */
    virtual void code_gen(Codegen& gen) {
        throw std::runtime_error("ASTNode code_gen called on bare ASTNode");
    }

    /**
     * virtual destructor for the ASTNode
     */
    virtual ~ASTNode() = default;

};