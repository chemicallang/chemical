// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class LambdaCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    LambdaCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompLambda;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "LambdaCST";
    }

#endif

};