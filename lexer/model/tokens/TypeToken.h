// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include <utility>

#include "LexToken.h"

class TypeToken : public LexToken {
public:

    std::string type;

    TypeToken(unsigned int start, unsigned int length, unsigned int lineNumber, std::string type) : LexToken(start, length, lineNumber), type(std::move(type)) {

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_type;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Type:");
        buf.append(type);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return type;
    }

};