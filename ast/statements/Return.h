// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class FunctionType;

class ReturnStatement : public ASTNode {
public:

    ASTNode* parent_node;
    FunctionType* func_type = nullptr;
    std::optional<std::unique_ptr<Value>> value;
    CSTToken* token;

    /**
     * @brief Construct a new ReturnStatement object.
     */
    ReturnStatement(
            std::optional<std::unique_ptr<Value>> value,
            FunctionType *declaration,
            ASTNode* parent_node,
            CSTToken* token
    );

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ReturnStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    BaseType* known_type() override;

    void interpret(InterpretScope &scope) override;

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) override;

#endif

};