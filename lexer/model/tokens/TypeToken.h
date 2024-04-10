// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "LexToken.h"
#include "AbstractStringToken.h"

class TypeToken : public AbstractStringToken {
public:

    TypeToken(const Position &position, std::string type) : AbstractStringToken(position, std::move(type)) {
        type.shrink_to_fit();
    }

    LexTokenType type() const override {
        return LexTokenType::Type;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

#ifdef LSP_BUILD
    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_type;
    }
#endif

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Type:");
        buf.append(value);
        return buf;
    }

};