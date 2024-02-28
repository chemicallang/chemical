// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "lexer/model/tokens/NumberToken.h"

class VarInitStatement : public ASTNode {
public:
    /**
     * @brief Construct a new InitStatement object.
     *
     * @param identifier The identifier being initialized.
     * @param value The value being assigned to the identifier.
     */
    VarInitStatement(
            std::string identifier,
            Value value
    ) : identifier(std::move(identifier)), value(std::move(value)) {}

private:
    std::string identifier; ///< The identifier being initialized.
    Value value; ///< The value being assigned to the identifier.
};