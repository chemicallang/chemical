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

    std::optional<std::string> declaration_identifier() override {
        return value;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_interface;
    }

    std::optional<lsCompletionItemKind> lsp_comp_kind() const override {
        return lsCompletionItemKind::Interface;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("InterfaceName:");
        buf.append(this->value);
        return buf;
    }

};