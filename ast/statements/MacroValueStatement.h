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

    /**
     * @brief Construct a new ReturnStatement object.
     */
    MacroValueStatement(std::string  name, std::unique_ptr<Value> value) : value(std::move(value)), name(std::move(name)) {}

    void interpret(InterpretScope &scope) override {
        value->evaluated_value(scope);
    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {

    }
#endif

    std::string representation() const override {
        return "#" + name + " " + value->representation() + " #end" + name;
    }

private:
    std::unique_ptr<Value> value;
    std::string name;

};