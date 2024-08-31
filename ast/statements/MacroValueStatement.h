// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

class MacroValueStatement : public ASTNode {
public:

    std::unique_ptr<Value> value;
    std::string name;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * @brief Construct a new ReturnStatement object.
     */
    MacroValueStatement(
        std::string name,
        std::unique_ptr<Value> value,
        ASTNode* parent_node,
        CSTToken* token
    ) : value(std::move(value)), name(std::move(name)), parent_node(parent_node), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void interpret(InterpretScope &scope) override {
        value->evaluated_value(scope);
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {

    }
#endif

};