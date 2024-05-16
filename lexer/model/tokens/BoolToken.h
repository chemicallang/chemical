// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class BoolToken : public LexToken {
public:

    using LexToken::LexToken;

    LexTokenType type() const override {
        return LexTokenType::Bool;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitBoolToken(this);
    }

};