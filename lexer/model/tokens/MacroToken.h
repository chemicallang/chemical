// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LexToken.h"

class MacroToken : public LexToken {
public:

    unsigned int len;

    SemanticTokenModifier modifier;

    MacroToken(const TokenPosition &position, unsigned int len, SemanticTokenModifier modifier) : LexToken(position), len(len), modifier(modifier) {
        // nothing
    }

    SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_macro;
    }

    LexTokenType type() const override {
        return LexTokenType::Macro;
    }

    std::string type_string() const override {
        return "Macro";
    }

    std::string representation() const override {
        return "[macro]";
    }

    std::string content() const override {
        return "";
    }

    unsigned int length() const override {
        return len;
    }

};