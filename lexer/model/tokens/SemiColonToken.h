// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

#include <utility>

class SemiColonToken : public LexToken {
public:

    SemiColonToken(unsigned int start, unsigned int length, unsigned int lineNumber) : LexToken(start, length, lineNumber) {

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_operator;
    }

    [[nodiscard]] std::string type_string() const override {
        return "SemiColon:;";
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};