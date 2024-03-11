// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class ModifierToken : public AbstractStringToken {
public:

    ModifierToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    LexTokenType type() const override {
        return LexTokenType::Modifier;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_modifier;
    }

    std::optional<std::string> lsp_comp_label() const override {
        return std::nullopt;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Modifier:");
        buf.append(this->value);
        return buf;
    }

};