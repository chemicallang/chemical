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

    inline char sec_last() {
        if(value.size() >= 2) {
            return value[value.size() - 2];
        } else {
            return '\0';
        }
    }

    inline char last() {
        return value[value.size() - 1];
    }

    bool is_float() {
        return last() == 'f' || last() == 'F';
    }

    bool is_long() {
        return last() == 'l' || last() == 'L';
    }

    bool is_unsigned() {
        return sec_last() == 'u' || sec_last() == 'U';
    }

    inline bool is_signed_long() {
        return !is_unsigned() && is_long();
    }

    inline bool is_unsigned_long() {
        return is_unsigned() && is_long();
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