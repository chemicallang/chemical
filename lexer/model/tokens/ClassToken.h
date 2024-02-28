// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class ClassToken : public AbstractStringToken {
public:

    ClassToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    TokenType type() const override {
        return TokenType::Class;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_class;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("ClassName:");
        buf.append(this->value);
        return buf;
    }

};