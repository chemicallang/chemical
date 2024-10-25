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
    SourceLocation location;

    /**
     * @brief Construct a new ReturnStatement object.
     */
    MacroValueStatement(
        std::string name,
        std::unique_ptr<Value> value,
        ASTNode* parent_node,
        SourceLocation token
    ) : value(std::move(value)), name(std::move(name)), parent_node(parent_node), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void interpret(InterpretScope &scope) final {
        value->evaluated_value(scope);
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final {

    }
#endif

};