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

    ASTNode* parent_node;
    CSTToken* token;

    /**
     * @brief Construct a new ForLoop object with an empty body
     */
//    ForLoop(
//            std::unique_ptr<VarInitStatement> initializer,
//            std::unique_ptr<Value> conditionExpr,
//            std::unique_ptr<ASTNode> incrementerExpr,
//            ASTNode* parent_node,
//            CSTToken* token
//    );

    ASTNodeKind kind() override {
        return ASTNodeKind::ForLoopStmt;
    }

    /**
     * @brief Construct a new ForLoop object.
     */
    ForLoop(
            std::unique_ptr<VarInitStatement> initializer,
            std::unique_ptr<Value> conditionExpr,
            std::unique_ptr<ASTNode> incrementerExpr,
            LoopScope body,
            ASTNode* parent_node,
            CSTToken* token
    );

    CSTToken* cst_token() override {
        return token;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope &scope) override;

    void stopInterpretation() override;

    std::unique_ptr<VarInitStatement> initializer;
    std::unique_ptr<Value> conditionExpr;
    std::unique_ptr<ASTNode> incrementerExpr;
    bool stoppedInterpretation = false;
};