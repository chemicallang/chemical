// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class UnionDefCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitUnionDef(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompUnionDef;
    }

};