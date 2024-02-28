// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class PropertyToken : public AbstractStringToken {
public:

    PropertyToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    TokenType type() const override {
        return TokenType::Property;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_property;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Property:");
        buf.append(this->value);
        return buf;
    }

};