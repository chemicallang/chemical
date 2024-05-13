// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class FunctionCallCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    FunctionCallCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitFunctionCall(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunctionCall;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "FunctionCallCST";
    }

#endif

};