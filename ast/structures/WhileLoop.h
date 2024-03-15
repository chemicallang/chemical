// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "Scope.h"
#include "LoopScope.h"
#include "ast/base/LoopASTNode.h"

class WhileLoop : public LoopASTNode {
public:

    /**
     * initializes the loop with only a condition and empty body
     * @param condition
     */
    WhileLoop(std::unique_ptr<Value> condition) : condition(std::move(condition)) {

    }

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(std::unique_ptr<Value> condition, LoopScope body)
            : condition(std::move(condition)), LoopASTNode(std::move(body)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void interpret(InterpretScope &scope) override {
        InterpretScope child(&scope, scope.global, &body, this);
        while(condition->evaluated_bool(child)){
            body.interpret(child);
            if(stoppedInterpretation) {
                stoppedInterpretation = false;
                break;
            }
        }
    }

    void code_gen(Codegen &gen) override {

        auto loopCond = llvm::BasicBlock::Create(*gen.ctx, "loopcond", gen.current_function);
        auto loopThen = llvm::BasicBlock::Create(*gen.ctx, "loopthen", gen.current_function);
        auto exitBlock = llvm::BasicBlock::Create(*gen.ctx, "loopexit", gen.current_function);

        // sending to loop condition
        gen.CreateBr(loopCond);

        // loop condition
        gen.SetInsertPoint(loopCond);
        auto comparison = condition->llvm_value(gen);
        gen.CreateCondBr(comparison, loopThen, exitBlock);

        // loop then
        gen.SetInsertPoint(loopThen);
        gen.loop_body_wrap(loopCond, exitBlock);
        body.code_gen(gen);
        gen.loop_body_wrap(loopCond, exitBlock);
        gen.CreateBr(loopCond);

        // loop exit
        gen.SetInsertPoint(exitBlock);

    }

    void stopInterpretation() override {
        stoppedInterpretation = true;
    }

    std::string representation() const override {
        std::string ret;
        ret.append("while(");
        ret.append(condition->representation());
        ret.append(") {\n");
        ret.append(body.representation());
        ret.append("\n}");
        return ret;
    }

private:
    std::unique_ptr<Value> condition;
    bool stoppedInterpretation = false;
};