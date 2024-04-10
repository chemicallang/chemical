// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class VariableToken : public AbstractStringToken {
public:

    VariableToken(const Position& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Variable;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_variable;
    }

    bool lsp_has_comp() const override {
        return !access;
    }

    std::optional<lsCompletionItemKind> lsp_comp_kind() const override {
        return lsCompletionItemKind::Variable;
    }

    std::optional<std::string> lsp_comp_label() const override {
        if(access) return std::nullopt; else return value;
    }
#endif

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Identifier:");
        buf.append(this->value);
        return buf;
    }

};