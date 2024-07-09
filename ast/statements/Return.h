// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class BaseFunctionType;

class ReturnStatement : public ASTNode {
public:

    ASTNode* parent_node;
    BaseFunctionType* func_type = nullptr;
    std::optional<std::unique_ptr<Value>> value;

    /**
     * @brief Construct a new ReturnStatement object.
     */
    ReturnStatement(
        std::optional<std::unique_ptr<Value>> value,
        BaseFunctionType *declaration,
        ASTNode* parent_node
    );

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void interpret(InterpretScope &scope) override;

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override;

    ReturnStatement *as_return() override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen, Scope *scope, unsigned int index) override;

#endif

};