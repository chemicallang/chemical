// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ArrayTypeCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ArrayTypeCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitArrayType(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompArrayType;
    }

};