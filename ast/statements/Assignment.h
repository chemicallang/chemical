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
    CSTToken* token;

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
            CSTToken* token
    );

    ASTNodeKind kind() override {
        return ASTNodeKind::AssignmentStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    CSTToken *cst_token() override {
        return token;
    }

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope& scope) override;

};