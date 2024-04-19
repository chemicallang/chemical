// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class NumberToken : public AbstractStringToken {
public:

    NumberToken(const Position &position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    bool has_dot() {
        return value.find('.') != std::string::npos;
    }

    bool is_float() {
        return value[value.size() - 1] == 'f';
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Number;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Number:");
        buf.append(value);
        return buf;
    }

};