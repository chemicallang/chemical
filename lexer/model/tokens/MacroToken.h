// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LexToken.h"
#include "lexer/minLsp/SemanticTokenModifier.h"

class MacroToken : public LexToken {
public:

    unsigned int len;

    LspSemanticTokenModifier modifier;

    MacroToken(const TokenPosition &position, unsigned int len, LspSemanticTokenModifier modifier) : LexToken(position), len(len), modifier(modifier) {
        // nothing
    }

    LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_macro;
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