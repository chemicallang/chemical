// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class DoWhileLoop : public ASTNode {
public:
    /**
     * @brief Construct a new DoWhileLoop object.
     *
     * @param body The body of the do-while loop.
     * @param condition The loop condition.
     */
    DoWhileLoop(std::shared_ptr<ASTNode> body, std::shared_ptr<ASTNode> condition)
            : body(body), condition(condition) {}

private:
    std::shared_ptr<ASTNode> body; ///< The body of the do-while loop.
    std::shared_ptr<ASTNode> condition; ///< The loop condition.
};