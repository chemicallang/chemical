// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class FunctionCallCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitFunctionCall(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunctionCall;
    }

};