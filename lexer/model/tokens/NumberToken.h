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

    LexTokenType type() const override {
        return LexTokenType::Number;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_number;
    }

    std::optional<std::string> lsp_comp_label() const override {
        return std::nullopt;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Number:");
        buf.append(value);
        return buf;
    }

};