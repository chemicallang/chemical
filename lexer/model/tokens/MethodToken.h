// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class MethodToken : public AbstractStringToken {
public:

    MethodToken(const Position& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    LexTokenType type() const override {
        return LexTokenType::Method;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_method;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("MethodName:");
        buf.append(this->value);
        return buf;
    }

};