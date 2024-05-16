// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"
#include "utils/StrUtils.h"

class NumberToken : public LexToken {
private:

    // check is always suppose to be lowercase
    static inline bool equal(char value, char check) {
        return check == value || value == check - 32;
    }

public:

    NumberToken(const Position &position, std::string value) : LexToken(position, std::move(value)) {

    }

    bool has_dot() {
        return value.find('.') != std::string::npos;
    }

    inline char safe_back_at(unsigned int pos) {
        if(value.size() >= pos) {
            return value[value.size() - pos];
        } else {
            return '\0';
        }
    }

    inline char sec_last() {
        return safe_back_at(2);
    }

    inline char last() {
        return value[value.size() - 1];
    }

    bool is_float() {
        return equal(last(), 'f');
    }

    bool is_long() {
        return equal(last(), 'l');
    }

    bool is_unsigned() {
        return equal(sec_last(), 'u');
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitNumberToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Number;
    }

};