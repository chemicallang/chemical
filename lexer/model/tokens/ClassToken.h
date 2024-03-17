// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "AbstractStringToken.h"

class ClassToken : public AbstractStringToken {
public:

    ClassToken(const Position& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    LexTokenType type() const override {
        return LexTokenType::Class;
    }

    [[nodiscard]] SemanticTokenType lspType() const override {
        return SemanticTokenType::ls_class;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("ClassName:");
        buf.append(this->value);
        return buf;
    }

};