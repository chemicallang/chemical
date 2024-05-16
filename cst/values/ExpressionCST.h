// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ExpressionCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ExpressionCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)){

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitExpression(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompExpression;
    }

};