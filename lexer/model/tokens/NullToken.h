// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"
#include "AbstractStringToken.h"

class NullToken : public AbstractStringToken {
public:

    NullToken(const Position &position) : AbstractStringToken(position, "null") {

    }

    LexTokenType type() const override {
        return LexTokenType::Null;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    [[nodiscard]] std::string type_string() const override {
        return "null";
    }

};