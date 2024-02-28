// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class VariableToken : public AbstractStringToken {
public:

    VariableToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)) {

    }

    LexTokenType type() const override {
        return LexTokenType::Variable;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_variable;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Identifier:");
        buf.append(this->value);
        return buf;
    }

};