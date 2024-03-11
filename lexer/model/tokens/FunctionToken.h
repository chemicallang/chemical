// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class FunctionToken : public AbstractStringToken {
public:

    unsigned int modifiers = 0;

    FunctionToken(const TokenPosition& position, std::string identifier, unsigned int modifiers) : AbstractStringToken(position, std::move(identifier)), modifiers(modifiers) {

    }

    std::optional<std::string> declaration_identifier() override {
        return value;
    }

    LexTokenType type() const override {
        return LexTokenType::Function;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_function;
    }

    std::optional<lsCompletionItemKind> lsp_comp_kind() const override {
        return lsCompletionItemKind::Function;
    }

    unsigned int lsp_modifiers() override {
        return modifiers;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("FunctionName:");
        buf.append(this->value);
        buf.append(";modifiers:" + std::to_string(modifiers));
        return buf;
    }

};