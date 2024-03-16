// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once


#include "AbstractStringToken.h"

class ParameterToken : public AbstractStringToken {
public:

    ParameterToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    LexTokenType type() const override {
        return LexTokenType::Parameter;
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_parameter;
    }

    // parameter has completions, but aren't supported yet !
    bool lsp_has_comp() const override {
        return false;
    }

    std::optional<std::string> lsp_comp_label() const override {
        return std::nullopt;
    }
#endif

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Parameter:");
        buf.append(this->value);
        return buf;
    }

};