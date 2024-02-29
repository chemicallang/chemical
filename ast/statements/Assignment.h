// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class AssignStatement : public ASTNode {
public:
    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    AssignStatement(
        std::string identifier,
        Value value
    ) : identifier(std::move(identifier)), value(std::move(value)) {}

private:
    std::string identifier; ///< The identifier being assigned.
    Value value; ///< The value being assigned to the identifier.
};