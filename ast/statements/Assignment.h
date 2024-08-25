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

    std::unique_ptr<Value> lhs;
    std::unique_ptr<Value> value;
    InterfaceDefinition* definition;
    Operation assOp;
    ASTNode* parent_node;

    /**
     * @brief Construct a new AssignStatement object.
     *
     * @param identifier The identifier being assigned.
     * @param value The value being assigned to the identifier.
     */
    AssignStatement(
            std::unique_ptr<Value> lhs,
            std::unique_ptr<Value> value,
            Operation assOp,
            ASTNode* parent_node
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

    void accept(Visitor *visitor) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

    void interpret(InterpretScope& scope) override;

};