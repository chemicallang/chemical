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

    void code_gen(Codegen &gen) override {

        // compare
        auto comparison = condition->llvm_value(gen);

        llvm::BasicBlock* elseBlock = nullptr;

        // blocks
        auto thenBlock = llvm::BasicBlock::Create(*gen.ctx, "then", gen.current_function);

        if(elseBody.has_value()) {
            elseBlock = llvm::BasicBlock::Create(*gen.ctx, "else", gen.current_function);
        }

        // end block
        auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "end", gen.current_function);

        // Branch based on comparison result
        gen.builder->CreateCondBr(comparison, thenBlock, elseBlock ? elseBlock : endBlock);

        // then code
        gen.builder->SetInsertPoint(thenBlock);
        ifBody.code_gen(gen);
        gen.builder->CreateBr(endBlock);

        // else block
        if(elseBlock) {
            gen.builder->SetInsertPoint(elseBlock);
            elseBody.value().code_gen(gen);
            gen.builder->CreateBr(endBlock);
        }

        // set to end block
        gen.builder->SetInsertPoint(endBlock);

    }

    void interpret(InterpretScope &scope) override {
        if(condition->evaluated_bool(scope)) {
            InterpretScope child(&scope, scope.global, &ifBody, this);
            ifBody.interpret(child);
        } else {
            for (auto const& elseIf:elseIfs) {
                if(elseIf->condition->evaluated_bool(scope)) {
                    InterpretScope child(&scope, scope.global, &elseIf->ifBody, this);
                    elseIf->ifBody.interpret(child);
                    return;
                }
            }
            if(elseBody.has_value()) {
                InterpretScope child(&scope, scope.global, &elseBody.value(), this);
                elseBody->interpret(child);
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