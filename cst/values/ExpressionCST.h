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
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompExpression;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "ExpressionCST";
    }

#endif

};