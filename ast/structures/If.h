// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class IfStatement : public ASTNode {
public:
    /**
     * @brief Construct a new IfStatement object.
     *
     * @param condition The condition of the if statement.
     * @param ifBody The body of the if statement.
     * @param elseBody The body of the else statement (can be nullptr if there's no else part).
     */
    IfStatement(std::shared_ptr<ASTNode> condition, std::shared_ptr<ASTNode> ifBody,
                std::shared_ptr<ASTNode> elseBody)
            : condition(condition), ifBody(ifBody), elseBody(elseBody) {}

private:
    std::shared_ptr<ASTNode> condition; ///< The condition of the if statement.
    std::shared_ptr<ASTNode> ifBody; ///< The body of the if statement.
    std::shared_ptr<ASTNode> elseBody; ///< The body of the else statement.
};