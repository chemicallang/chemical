// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class NumberToken : public AbstractStringToken {
public:

    NumberToken(const TokenPosition &position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    bool has_dot() {
        return value.find('.') != std::string::npos;
    }

    // TODO everything is a double, at the moment
    bool is_float() {
        return false;
    }

    LexTokenType type() const override {
        return LexTokenType::Number;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_number;
    }

    bool lsp_has_comp() const override {
        return false;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Number:");
        buf.append(value);
        return buf;
    }

};