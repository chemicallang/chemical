// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class TypealiasCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitTypealias(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompTypealias;
    }

};