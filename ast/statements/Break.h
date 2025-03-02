// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class BreakStatement : public ASTNode {
private:
    LoopASTNode *node;
public:

    /**
     * sometimes a break statement can break with a value, in cases where break is within
     * a loop block, that is itself a value, so this value is assigned to the variable owning the loop
     * block and then the loop is broken
     */
    Value* value;

    /**
     * constructor
     */
    constexpr BreakStatement(
        LoopASTNode *node,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::BreakStmt, parent_node, location), node(node), value(nullptr) {

    }

    /**
     * constructor
     */
    constexpr BreakStatement(
            Value* value,
            LoopASTNode *node,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::BreakStmt, parent_node, location), node(node), value(value) {

    }

    ASTNode* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<BreakStatement>()) BreakStatement(
            value ? value->copy(allocator) : nullptr,
            node,
            parent(),
            encoded_location()
        );
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void interpret(InterpretScope &scope) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};