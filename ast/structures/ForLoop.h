// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "Scope.h"
#include "ast/statements/VarInit.h"

class ForLoop : public ASTNode {
public:

    /**
     * @brief Construct a new ForLoop object.
     */
    ForLoop(
            std::unique_ptr<VarInitStatement> initializer,
            std::unique_ptr<Value> conditionExpr,
            std::unique_ptr<ASTNode> incrementerExpr,
            LoopScope body
    ) : initializer(std::move(initializer)),
        conditionExpr(std::move(conditionExpr)), incrementerExpr(std::move(incrementerExpr)),
        body(std::move(body)) {}

    void interpret(InterpretScope &scope) override {
        InterpretScope child(&scope, scope.global, &body, this);
        initializer->interpret(child);
        while(conditionExpr->evaluated_bool(child)){
            body.interpret(child);
            if(stoppedInterpretation){
                stoppedInterpretation = false;
                break;
            }
            incrementerExpr->interpret(child);
        }
    }

    bool supportsBreak() override {
        return true;
    }

    bool supportsContinue() override {
        return true;
    }

    void stopInterpretation() override {
        stoppedInterpretation = true;
    }

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
    std::unique_ptr<ASTNode> incrementerExpr;
    LoopScope body;
    bool stoppedInterpretation = false;
};