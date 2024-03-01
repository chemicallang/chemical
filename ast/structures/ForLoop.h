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
     */
    ForLoop(
            std::unique_ptr<VarInitStatement> initializer,
            std::unique_ptr<Value> conditionExpr,
            std::unique_ptr<Value> incrementerExpr,
            Scope body
    ) : initializer(std::move(initializer)),
        conditionExpr(std::move(conditionExpr)), incrementerExpr(std::move(incrementerExpr)),
        body(std::move(body)) {}

    std::string representation() const override {
        std::string ret("for(");
        ret.append(initializer->representation());
        ret.append(1, ';');
        ret.append(conditionExpr->representation());
        ret.append(1, ';');
        ret.append(incrementerExpr->representation());
        ret.append("{\n");
        ret.append(body.representation());
        ret.append("\n}");
        return ret;
    }

private:
    std::unique_ptr<VarInitStatement> initializer;
    std::unique_ptr<Value> conditionExpr;
    std::unique_ptr<Value> incrementerExpr;
    Scope body;
};