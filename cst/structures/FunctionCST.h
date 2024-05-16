// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class FunctionParamCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitFunctionParam(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunctionParam;
    }

};

class FunctionCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    FunctionCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitFunction(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunction;
    }

};