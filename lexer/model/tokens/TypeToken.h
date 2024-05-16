// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class TypeToken : public LexToken {
public:

    TypeToken(const Position &position, std::string type) : LexToken(position, std::move(type)) {

    }

    LexTokenType type() const override {
        return LexTokenType::Type;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

};