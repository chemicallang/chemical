// Copyright (c) Qinetik 2024.

#pragma once

#include "LexToken.h"

class LexUserToken : public LexToken {
public:

    LexUserToken(Position position, std::string value) : LexToken(position, std::move(value)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitUserToken(this);
    }

    LexTokenType type() const override {
        return LexTokenType::UserToken;
    }

};