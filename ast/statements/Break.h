// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class BreakStatement : public ASTNode {
public:
    /**
     * @brief Construct a new InitStatement object.
     *
     * @param identifier The identifier being initialized.
     * @param value The value being assigned to the identifier.
     */
    BreakStatement(
            std::unique_ptr<ASTNode> value
    ) : value(std::move(value)) {}

private:
    std::unique_ptr<ASTNode> value; ///< The value being assigned to the identifier.
};