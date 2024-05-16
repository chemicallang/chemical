// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class IdentifierToken : public LexToken {
public:

    IdentifierToken(const Position& position, std::string identifier) : LexToken(position, std::move(identifier)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Identifier;
    }

};