// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "Scope.h"
#include "ast/base/Value.h"
#include <optional>

class IfStatement : public ASTNode {
public:
    /**
     * @brief Construct a new IfStatement object.
     *
     * @param condition The condition of the if statement.
     * @param ifBody The body of the if statement.
     * @param elseBody The body of the else statement (can be nullptr if there's no else part).
     */
    IfStatement(
            std::unique_ptr<Value> condition,
            Scope ifBody,
            std::vector<std::unique_ptr<IfStatement>> elseIfs,
            std::optional<Scope> elseBody
    ) : condition(std::move(condition)), ifBody(std::move(ifBody)),
        elseIfs(std::move(elseIfs)), elseBody(std::move(elseBody)) {}

    void interpret(InterpretScope &scope) override {
        auto cond = condition->evaluated_value(scope.values);
        if(cond->as_bool()) {
            ifBody.interpret(scope);
        } else {
            delete cond;
            for (auto const& elseIf:elseIfs) {
                cond = elseIf->condition->evaluated_value(scope.values);
                if(cond->as_bool()) {
                    elseIf->ifBody.interpret(scope);
                    return;
                }
            }
            if(elseBody.has_value()) {
                elseBody->interpret(scope);
            }
        }
    }

    std::string representation() const override {
        std::string rep;
        rep.append("if(");
        rep.append(condition->representation());
        rep.append("){\n");
        rep.append(ifBody.representation());
        rep.append("\n}");
        int i = 0;
        while(i < elseIfs.size()) {
            rep.append("else ");
            rep.append(elseIfs[i]->representation());
            i++;
        }
        if(elseBody.has_value()) {
            rep.append("else {\n");
            rep.append(elseBody.value().representation());
            rep.append("\n}");
        }
        return rep;
    }

private:
    std::unique_ptr<Value> condition;
    Scope ifBody;
    std::vector<std::unique_ptr<IfStatement>> elseIfs;
    std::optional<Scope> elseBody;
};