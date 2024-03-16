// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class EnumToken : public AbstractStringToken {
public:

    EnumToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    LexTokenType type() const override {
        return LexTokenType::Enum;
    }

    std::optional<std::string> declaration_identifier() override {
        return value;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_enum;
    }

    std::optional<lsCompletionItemKind> lsp_comp_kind() const override {
        return lsCompletionItemKind::Enum;
    }
#endif

    [[nodiscard]] std::string type_string() const override {
        std::string buf("EnumName:");
        buf.append(this->value);
        return buf;
    }

};