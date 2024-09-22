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
    VarInitStatement* initializer;
    Value* conditionExpr;
    ASTNode* incrementerExpr;
    bool stoppedInterpretation = false;

    /**
     * @brief Construct a new ForLoop object.
     */
    ForLoop(
            VarInitStatement* initializer,
            Value* conditionExpr,
            ASTNode* incrementerExpr,
            LoopScope body,
            ASTNode* parent_node,
            CSTToken* token
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::ForLoopStmt;
    }

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

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope &scope) override;

    void stopInterpretation() override;

};