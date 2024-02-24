// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"

#include <utility>

class IdentifierToken : public LexToken {
public:

    std::string identifier;

    IdentifierToken(unsigned int start, std::string identifier, unsigned int lineNumber) : LexToken(start, lineNumber), identifier(std::move(identifier)) {

    }

    unsigned int length() const override {
        return identifier.length();
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_variable;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Identifier:");
        buf.append(this->identifier);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};