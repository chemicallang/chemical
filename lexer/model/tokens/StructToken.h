// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class StructToken : public AbstractStringToken {
public:

    StructToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    LexTokenType type() const override {
        return LexTokenType::Struct;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_struct;
    }

    std::optional<lsCompletionItemKind> lsp_comp_kind() const override {
        return lsCompletionItemKind::Struct;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Struct:");
        buf.append(this->value);
        return buf;
    }

};