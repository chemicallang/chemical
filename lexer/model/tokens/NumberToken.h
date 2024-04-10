// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class NumberToken : public AbstractStringToken {
public:

    NumberToken(const Position &position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    bool has_dot() {
        return value.find('.') != std::string::npos;
    }

    // TODO everything is a double, at the moment
    bool is_float() {
        return false;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::Number;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_number;
    }

    bool lsp_has_comp() const override {
        return false;
    }
#endif

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Number:");
        buf.append(value);
        return buf;
    }

};