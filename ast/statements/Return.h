// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class FunctionType;

class ReturnStatement : public ASTNode {
public:

    FunctionTypeBody* func_type = nullptr;
    Value* value;

    /**
     * Construct a new ReturnStatement object.
     */
    constexpr ReturnStatement(
            Value* value,
            FunctionTypeBody* declaration,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::ReturnStmt, parent_node, location), value(value), func_type(declaration) {

    }

    ReturnStatement* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ReturnStatement>()) ReturnStatement(
            value->copy(allocator),
            nullptr,
            parent(),
            encoded_location()
        );
    }

    BaseType* known_type() final;

    void interpret(InterpretScope &scope) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) final;

#endif

};