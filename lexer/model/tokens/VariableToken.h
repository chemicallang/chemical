// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class VariableToken : public LexToken {
public:

    VariableToken(const Position& position, std::string identifier) : LexToken(position, std::move(identifier)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Variable;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Identifier:");
        buf.append(this->value);
        return buf;
    }

};