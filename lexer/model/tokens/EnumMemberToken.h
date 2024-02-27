// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "AbstractStringToken.h"

class EnumMemberToken : public AbstractStringToken {
public:

    EnumMemberToken(const TokenPosition& position, std::string identifier) : AbstractStringToken(position, std::move(identifier)){

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_enumMember;
    }

    [[nodiscard]] std::string type_string() const override {
        std::string buf("EnumMember:");
        buf.append(this->value);
        return buf;
    }

};