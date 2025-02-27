// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/utils/Operation.h"
#include <utility>

class AssignStatement : public ASTNode {
public:

    Value* lhs;
    Value* value;
    InterfaceDefinition* definition;
    Operation assOp;
    ASTNode* parent_node;

    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    constexpr AssignStatement(
            Value* lhs,
            Value* value,
            Operation assOp,
            ASTNode* parent_node,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::AssignmentStmt, location), lhs(lhs), value(value), assOp(assOp), parent_node(parent_node) {

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

    void interpret(InterpretScope& scope) final;

};