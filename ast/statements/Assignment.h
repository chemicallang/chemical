// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

class AssignStatement : public ASTNode {
public:

    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    AssignStatement(
            std::unique_ptr<AccessChain> lhs,
            std::unique_ptr<Value> value
    ) : lhs(std::move(lhs)), value(std::move(value)) {}

private:
    std::unique_ptr<AccessChain> lhs; ///< The identifier being assigned.
    std::unique_ptr<Value> value; ///< The value being assigned to the identifier.
};