// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class WhileLoop : public ASTNode {
public:
    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(std::shared_ptr<ASTNode> condition, std::shared_ptr<ASTNode> body)
            : condition(condition), body(body) {}

private:
    std::shared_ptr<ASTNode> condition; ///< The loop condition.
    std::shared_ptr<ASTNode> body; ///< The body of the while loop.
};