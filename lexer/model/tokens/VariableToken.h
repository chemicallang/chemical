// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class VariableToken : public LexToken {
public:

    /**
     * the linked token, it can be null
     * that's the difference between identifier and variable tokens, variable
     * tokens can be linked with their definitions (e.g a var init statement)
     * to link the tokens, CSTSymbolResolver is used.
     */
    CSTToken* linked;

    using LexToken::LexToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitVariableToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Variable;
    }

};