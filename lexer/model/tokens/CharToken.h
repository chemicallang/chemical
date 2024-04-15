// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class CharToken : public LexToken {
public:

    char value;

    unsigned int len;

    CharToken(const Position& position, char value, unsigned int length) : LexToken(position), value(value), len(length) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    unsigned int length() const override {
        return len;
    }

    LexTokenType type() const override {
        return LexTokenType::Char;
    }

    void append_representation(std::string &rep) const override {
        rep.append(1, '\'');
        rep.append(1,value);
        rep.append(1, '\'');
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Char:");
        buf.append(1, value);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        std::string buf;
        buf.append(1, value);
        return buf;
    }

};