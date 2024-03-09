// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class InterfaceToken : public AbstractStringToken {
public:

    InterfaceToken(const TokenPosition &position, std::string identifier)
            : AbstractStringToken(position, std::move(identifier)) {}

    LexTokenType type() const override {
        return LexTokenType::Interface;
    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_interface;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("InterfaceName:");
        buf.append(this->value);
        return buf;
    }

};