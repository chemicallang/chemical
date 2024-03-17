// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class FunctionToken : public AbstractStringToken {
public:

    FunctionToken(const Position& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)) {

    }

    std::optional<std::string> declaration_identifier() override {
        return value;
    }

    LexTokenType type() const override {
        return LexTokenType::Function;
    }

#ifdef LSP_BUILD
    unsigned int modifiers = 0;

    FunctionToken(const Position& position, std::string identifier, unsigned int modifiers) : AbstractStringToken(position, std::move(identifier)), modifiers(modifiers) {

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
#endif

    [[nodiscard]] std::string type_string() const override {
        std::string buf("FunctionName:");
        buf.append(this->value);
        return buf;
    }

};