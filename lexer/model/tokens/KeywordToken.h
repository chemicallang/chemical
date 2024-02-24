// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

#include <utility>

class KeywordToken : public LexToken {
public:

    std::string keyword;

    unsigned int len;

    KeywordToken(unsigned int start, unsigned int length, unsigned int lineNumber, std::string keyword) : LexToken(start, lineNumber), len(length), keyword(std::move(keyword)) {

    }

    unsigned int length() const override {
        return len;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_keyword;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Keyword:");
        buf.append(this->keyword);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};