// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class EnumMemberToken : public AbstractStringToken {
public:

    EnumMemberToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    LexTokenType type() const override {
        return LexTokenType::EnumMember;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_enumMember;
    }

    std::optional<lsCompletionItemKind> lsp_comp_kind() const override {
        return lsCompletionItemKind::EnumMember;
    }
#endif

    [[nodiscard]] std::string type_string() const override {
        std::string buf("EnumMember:");
        buf.append(this->value);
        return buf;
    }

};