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
     * code_gen function that generates llvm Value
     * @return
     */
    virtual llvm::Value* code_gen() {
        // TODO make this = 0 when every node complies
        return nullptr;
    }

    /**
     * virtual destructor for the ASTNode
     */
    virtual ~ASTNode() = default;

};