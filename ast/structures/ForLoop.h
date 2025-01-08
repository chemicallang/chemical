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
    SourceLocation location;
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
    );

    ASTNodeKind kind() final {
        return ASTNodeKind::ForLoopStmt;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void interpret(InterpretScope &scope) final;

    void stopInterpretation() final;

};