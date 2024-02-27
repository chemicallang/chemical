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

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_struct;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("Struct:");
        buf.append(this->value);
        return buf;
    }

};