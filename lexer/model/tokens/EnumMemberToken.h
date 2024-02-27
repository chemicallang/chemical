// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/12/2023.
//

#pragma once

#include "IdentifierToken.h"

class EnumMemberToken : public IdentifierToken {
public:

    EnumMemberToken(const TokenPosition& position, std::string identifier) : IdentifierToken(position, std::move(identifier)){

    }

    [[nodiscard]] LspSemanticTokenType lspType() const override {
        return LspSemanticTokenType::ls_enumMember;
    }

};