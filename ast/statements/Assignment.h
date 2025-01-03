#include <utility>

// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"
#include "ast/utils/Operation.h"

class AssignStatement : public ASTNode {
public:

    Value* lhs;
    Value* value;
    InterfaceDefinition* definition;
    Operation assOp;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    AssignStatement(
            Value* lhs,
            Value* value,
            Operation assOp,
            ASTNode* parent_node,
            SourceLocation location
    );

    ASTNodeKind kind() final {
        return ASTNodeKind::AssignmentStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    void accept(Visitor *visitor) final;

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final;
#endif

    void interpret(InterpretScope& scope) final;

};