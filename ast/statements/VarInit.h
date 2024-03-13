// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "lexer/model/tokens/NumberToken.h"
#include <optional>

class VarInitStatement : public ASTNode, public Value {
public:

    /**
     * @brief Construct a new InitStatement object.
     *
     * @param identifier The identifier being initialized.
     * @param value The value being assigned to the identifier.
     */
    VarInitStatement(
            std::string identifier,
            std::optional<std::string> type,
            std::optional<std::unique_ptr<Value>> value
    ) : identifier(std::move(identifier)), type(std::move(type)), value(std::move(value)) {}

    void code_gen(Codegen &gen) override {
        if(!type.has_value() && !value.has_value()) {
            gen.error("neither variable type no variable value were given");
            return;
        }
        auto llvm_type = value.has_value() ? value.value()->llvm_type(gen) : gen.llvm_type(type.value());
        auto x = gen.builder->CreateAlloca(llvm_type, nullptr, identifier);
        if(value.has_value()) {
            gen.builder->CreateStore(value.value()->llvm_value(gen), x);
        }
    }

    void interpret(InterpretScope& scope) override {
        if(value.has_value()) {
            if (value.value()->primitive()) {
                scope.values[identifier] = value.value()->copy();
            } else {
//            std::cerr << "First Non-Copied Var : " << identifier;
                scope.values[identifier] = value.value()->evaluated_value(scope);
            }
        } else {
            scope.values[identifier] = this;
        }
    }

    std::string representation() const override {
        std::string rep;
        rep.append("var ");
        rep.append(identifier);
        if(type.has_value()) {
            rep.append(" : ");
            rep.append(type.value());
        }
        if(value.has_value()) {
            rep.append(" = ");
            rep.append(value.value()->representation());
        }
        return rep;
    }

    void scope_ends() override {
        // do not call destructor
    }

    std::string identifier; ///< The identifier being initialized.
private:
    std::optional<std::string> type;
    std::optional<std::unique_ptr<Value>> value; ///< The value being assigned to the identifier.

};