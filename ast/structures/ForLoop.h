// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "Scope.h"
#include "ast/statements/VarInit.h"

class ForLoop : public LoopASTNode {
public:

    /**
     * @brief Construct a new ForLoop object with an empty body
     */
    ForLoop(
            std::unique_ptr<VarInitStatement> initializer,
            std::unique_ptr<Value> conditionExpr,
            std::unique_ptr<ASTNode> incrementerExpr
    ) : initializer(std::move(initializer)),
        conditionExpr(std::move(conditionExpr)), incrementerExpr(std::move(incrementerExpr)) {}

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
        LoopASTNode(std::move(body)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {

        // initialize the variables
        initializer->code_gen(gen);

        // creating blocks
        auto condBlock = llvm::BasicBlock::Create(*gen.ctx, "forcond", gen.current_function);
        auto thenBlock = llvm::BasicBlock::Create(*gen.ctx, "forthen", gen.current_function);
        auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "forend", gen.current_function);

        // going to condition
        gen.CreateBr(condBlock);

        // condition block
        gen.SetInsertPoint(condBlock);
        auto comparison = conditionExpr->llvm_value(gen);
        gen.CreateCondBr(comparison, thenBlock, endBlock);

        // then block
        gen.SetInsertPoint(thenBlock);
        gen.loop_body_wrap(condBlock, endBlock);
        body.code_gen(gen);
        gen.loop_body_wrap(condBlock, endBlock);
        incrementerExpr->code_gen(gen);
        gen.CreateBr(condBlock);

        // end block
        gen.SetInsertPoint(endBlock);

    }
#endif

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
        initializer->interpret_scope_ends(child);
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

    std::unique_ptr<VarInitStatement> initializer;
    std::unique_ptr<Value> conditionExpr;
    std::unique_ptr<ASTNode> incrementerExpr;
    bool stoppedInterpretation = false;
};