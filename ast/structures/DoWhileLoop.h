// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/LoopASTNode.h"
#include "Scope.h"

class DoWhileLoop : public LoopASTNode {
private:

    bool stoppedInterpretation = false;

public:

    Value* condition;
    ASTNode* parent_node;

    /**
     * constructor
     */
    constexpr DoWhileLoop(
            Value* condition,
            ASTNode* parent_node,
            SourceLocation location
    ) : condition(condition), LoopASTNode(ASTNodeKind::DoWhileLoopStmt, location), parent_node(parent_node) {

    }


    ASTNode * parent() final {
        return parent_node;
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void interpret(InterpretScope &scope) final;

    void stopInterpretation() final;

};