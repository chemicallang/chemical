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

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
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
        std::string rep("Macro:");
        append_representation(rep);
        return rep;
    }

    void append_representation(std::string &rep) const override {
        if(isAnnotation) {
            rep.append(1, '@');
        } else {
            rep.append(1, '#');
        }
        rep.append(name);
    }

    std::string content() const override {
        return "";
    }

    unsigned int length() const override {
        return name.length() + 1;
    }

};