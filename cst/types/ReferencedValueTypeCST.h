// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ReferencedValueTypeCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitReferencedValueType(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompReferencedValueType;
    }

};