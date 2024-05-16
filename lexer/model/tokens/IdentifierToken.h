// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class IdentifierToken : public LexToken {
public:

    using LexToken::LexToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitIdentifierToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Identifier;
    }

};