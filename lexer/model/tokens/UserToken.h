// Copyright (c) Qinetik 2024.

#pragma once

#include "LexToken.h"

class LexUserToken : public AbstractStringToken {
public:

    LexUserToken(Position position, std::string value) : AbstractStringToken(position, std::move(value)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::UserToken;
    }

};