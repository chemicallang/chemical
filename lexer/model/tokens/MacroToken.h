// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LexToken.h"

class MacroToken : public LexToken {
public:

    std::string name;

    MacroToken(
            const Position &position,
            std::string name
    ) : LexToken(position), name(std::move(name)) {
        // nothing
    }

#ifdef LSP_BUILD
    SemanticTokenModifier modifier;

    MacroToken(const Position &position, unsigned int len, SemanticTokenModifier modifier) : LexToken(position), len(len), modifier(modifier) {
        // nothing
    }

    SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_macro;
    }
#endif

    LexTokenType type() const override {
        return LexTokenType::Macro;
    }

    std::string type_string() const override {
        return "Annotation";
    }

    std::string representation() const override {
        return "@" + name;
    }

    std::string content() const override {
        return "";
    }

    unsigned int length() const override {
        return name.length() + 1;
    }

};