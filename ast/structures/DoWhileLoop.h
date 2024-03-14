// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/LoopASTNode.h"
#include "Scope.h"

class DoWhileLoop : public LoopASTNode {
public:

    /**
     * Initialize an empty do while loop
     */
    DoWhileLoop() {

    }

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    DoWhileLoop(std::unique_ptr<Value> condition, LoopScope body)
            : condition(std::move(condition)), LoopASTNode(std::move(body)) {}

    void code_gen(Codegen &gen) override {

        auto loopThen = llvm::BasicBlock::Create(*gen.ctx, "loopthen", gen.current_function);
        auto loopCond = llvm::BasicBlock::Create(*gen.ctx, "loopcond", gen.current_function);
        auto exitBlock = llvm::BasicBlock::Create(*gen.ctx, "loopexit", gen.current_function);

        // sending to loop then
        gen.CreateBr(loopThen);

        // loop then
        gen.SetInsertPoint(loopThen);
        gen.loop_body_wrap(loopCond, exitBlock);
        body.code_gen(gen);
        gen.loop_body_wrap(loopCond, exitBlock);
        gen.CreateBr(loopCond);

        // loop condition
        gen.SetInsertPoint(loopCond);
        auto comparison = condition->llvm_value(gen);
        gen.CreateCondBr(comparison, loopThen, exitBlock);

        // loop exit
        gen.SetInsertPoint(exitBlock);

    }

    void interpret(InterpretScope &scope) override {
        InterpretScope child(&scope, scope.global, &body, this);
        do {
            body.interpret(child);
            if (stoppedInterpretation) {
                stoppedInterpretation = false;
                break;
            }
        } while (condition->evaluated_bool(child));
    }

    void stopInterpretation() override {
        stoppedInterpretation = true;
    }

    std::string representation() const override {
        std::string ret;
        ret.append("do {\n");
        ret.append(body.representation());
        ret.append("\n} while (");
        ret.append(condition->representation());
        ret.append(")");
        return ret;
    }

    std::unique_ptr<Value> condition;

private:
    bool stoppedInterpretation = false;
};