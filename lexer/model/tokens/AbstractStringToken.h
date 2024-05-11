// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "LexToken.h"

/**
 * this token should be inherited, when a normal identifier / string
 * can mean different things like a variable name, enum name, method name, function name
 */
class AbstractStringToken : public LexToken {
public:

    std::string value;

    AbstractStringToken(const Position &position, std::string value) : LexToken(position), value(std::move(value)) {
        value.shrink_to_fit();
    }

    unsigned int length() const override {
        return value.length();
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitStringCommon(this);
    }

    void append_representation(std::string &rep) const override {
        rep.append(value);
    }

};