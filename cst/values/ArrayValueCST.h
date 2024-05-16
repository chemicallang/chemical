// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ArrayValueCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ArrayValueCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitArrayValue(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompArrayValue;
    }

};