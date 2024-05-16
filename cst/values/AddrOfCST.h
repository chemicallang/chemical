// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class AddrOfCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitAddrOf(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompAddrOf;
    }

};