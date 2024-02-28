// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class MethodToken : public AbstractStringToken {
public:

    MethodToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    TokenType type() const override {
        return TokenType::Method;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_method;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("MethodName:");
        buf.append(this->value);
        return buf;
    }

};