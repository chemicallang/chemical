// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "LexToken.h"

/**
 * this token should be inherited, when a normal identifier / string
 * can mean different things like a variable name, enum name, method name, function name
 */
class AbstractStringToken : public LexToken {
public:

    std::string value;

    AbstractStringToken(const Position &position, std::string value) : LexToken(position), value(std::move(value)) {
        value.shrink_to_fit();
    }

    bool is_abs_string() override {
        return true;
    }

    unsigned int length() const override {
        return value.length();
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitStringCommon(this);
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_keyword;
    }

    bool lsp_has_comp() const override {
        return true;
    }

    std::optional<std::string> lsp_comp_label() const override {
        return value;
    }
#endif

    void append_representation(std::string &rep) const override {
        rep.append(value);
    }

    [[nodiscard]] std::string content() const override {
        return "";
    }

};