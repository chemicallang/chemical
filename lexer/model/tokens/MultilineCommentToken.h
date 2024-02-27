// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 25/02/2024.
//

#pragma once

#include "LexToken.h"

class MultilineCommentToken : public LexToken {
public:

    std::string value;

    MultilineCommentToken(const TokenPosition& position, std::string value) : LexToken(position), value(std::move(value)) {

    }

    unsigned int length() const override {
        // 4 is added because the token start has /* and then comment and then */ which is value
        return value.length() + 4;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_string;
    }

    std::string representation() const override {
        return "/* " + value + "*/";
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Comment:");
        buf.append(value);
        return buf;
    }

    [[nodiscard]] std::string content() const override {
        return value;
    }

};