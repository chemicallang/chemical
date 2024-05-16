// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LexToken.h"

class MacroToken : public LexToken {
public:

    using LexToken::LexToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitMacroToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Macro;
    }

};