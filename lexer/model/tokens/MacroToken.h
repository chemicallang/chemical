// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LexToken.h"

class MacroToken : public LexToken {
public:

    std::string name;
    bool isAnnotation;
    bool isMacroEnd;

    MacroToken(
            const Position &position,
            std::string name,
            bool isAnnotation,
            bool isMacroEnd = false
    ) : LexToken(position), name(std::move(name)), isAnnotation(isAnnotation), isMacroEnd(isMacroEnd) {
        // nothing
    }

#ifdef LSP_BUILD
    SemanticTokenModifier modifier;

    MacroToken(
            const Position &position,
            std::string name,
            bool isAnnotation,
            SemanticTokenModifier modifier
    ) : LexToken(position), name(std::move(name)), isAnnotation(isAnnotation), modifier(modifier) {
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
        return "Macro:" + representation();
    }

    std::string representation() const override {
        if(isAnnotation) {
            return "@" + name;
        } else {
            return "#" + name;
        }
    }

    std::string content() const override {
        return "";
    }

    unsigned int length() const override {
        return name.length() + 1;
    }

};