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

    Value* condition;
    bool stoppedInterpretation = false;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * initializes the loop with only a condition and empty body
     * @param condition
     */
//    WhileLoop(std::unique_ptr<Value> condition, ASTNode* parent_node, CSTToken* token);

    /**
     * @brief Construct a new WhileLoop object.
     *
     * @param condition The loop condition.
     * @param body The body of the while loop.
     */
    WhileLoop(Value* condition, LoopScope body, ASTNode* parent_node, CSTToken* token);

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::WhileLoopStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void interpret(InterpretScope &scope) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void stopInterpretation() override;

};