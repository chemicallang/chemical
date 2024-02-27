// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

class KeywordToken : public LexToken {
public:

    std::string keyword;

    KeywordToken(const TokenPosition& position, std::string keyword) : LexToken(position), keyword(std::move(keyword)) {

    }

    unsigned int length() const override {
        return keyword.length();
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_keyword;
    }

    std::string representation() const override {
        return this->keyword;
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