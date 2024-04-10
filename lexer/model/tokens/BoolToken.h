// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class BoolToken : public LexToken {
public:

    bool value;

    BoolToken(const Position& position, bool value) : LexToken(position), value(value) {

    }

    LexTokenType type() const override {
        return LexTokenType::Bool;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_keyword;
    }
#endif

    unsigned int length() const override {
        if(value) {
            return 4;
        } else {
            return 5;
        }
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    void append_representation(std::string &rep) const override {
        if(value) {
            rep.append("true");
        } else {
            rep.append("false");
        }
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Bool:");
        if(value) {
            buf.append("true");
        } else {
            buf.append("false");
        }
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};