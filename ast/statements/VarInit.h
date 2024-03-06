// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "lexer/model/tokens/NumberToken.h"
#include <optional>

class VarInitStatement : public ASTNode {
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
            std::unique_ptr<Value> value
    ) : identifier(std::move(identifier)), type(std::move(type)), value(std::move(value)) {}

    void interpret(InterpretScope& scope) override {
        scope.values[identifier] = value->evaluated_value(scope);
    }

    std::string representation() const override {
        std::string rep;
        rep.append("var ");
        rep.append(identifier);
        if(type.has_value()) {
            rep.append(" : ");
            rep.append(type.value());
        }
        rep.append(" = ");
        rep.append(value->representation());
        return rep;
    }


    std::string identifier; ///< The identifier being initialized.
private:
    std::optional<std::string> type;
    std::unique_ptr<Value> value; ///< The value being assigned to the identifier.

};