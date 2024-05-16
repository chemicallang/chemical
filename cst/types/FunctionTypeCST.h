// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class FunctionTypeCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitFunctionType(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunctionType;
    }

};