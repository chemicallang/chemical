// Copyright (c) Chemical Language Foundation 2025.

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
    ASTNode* initializer;
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
            Scope body,
            ASTNode* parent_node,
            SourceLocation location
    ) : LoopASTNode(std::move(body), ASTNodeKind::ForLoopStmt, location), initializer(initializer), conditionExpr(conditionExpr),
        incrementerExpr(incrementerExpr), parent_node(parent_node) {

    }


    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void interpret(InterpretScope &scope) final;

    void stopInterpretation() final;

};