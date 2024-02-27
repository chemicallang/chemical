// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class ForLoop : public ASTNode {
public:
    /**
     * @brief Construct a new ForLoop object.
     *
     * @param initialization The initialization statement of the for loop.
     * @param condition The loop condition.
     * @param update The loop update statement.
     * @param body The body of the for loop.
     */
    ForLoop(std::shared_ptr<ASTNode> initialization, std::shared_ptr<ASTNode> condition,
            std::shared_ptr<ASTNode> update, std::shared_ptr<ASTNode> body)
            : initialization(initialization), condition(condition), update(update), body(body) {}

private:
    std::shared_ptr<ASTNode> initialization; ///< The initialization statement of the for loop.
    std::shared_ptr<ASTNode> condition; ///< The loop condition.
    std::shared_ptr<ASTNode> update; ///< The loop update statement.
    std::shared_ptr<ASTNode> body; ///< The body of the for loop.
};