#include <utility>

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
            std::unique_ptr<Value> value,
            Operation assOp
    ) : lhs(std::move(lhs)), value(std::move(value)), assOp(assOp) {}

    void interpret(InterpretScope& scope) override {
        if(assOp == Operation::Equal) {
            lhs->set_in_parent(scope.values, value->evaluated_value(scope.values));
        } else {
            lhs->set_in_parent(scope.values, value->evaluated_value(scope.values), assOp);
        }
    }

    std::string representation() const override {
        std::string rep;
        rep.append(lhs->representation());
        rep.append(" = ");
        rep.append(value->representation());
        return rep;
    }

private:
    std::unique_ptr<AccessChain> lhs; ///< The identifier being assigned.
    std::unique_ptr<Value> value; ///< The value being assigned to the identifier.
    Operation assOp;
};