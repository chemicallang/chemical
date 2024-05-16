// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class DoWhileCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitDoWhile(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompDoWhile;
    }

};