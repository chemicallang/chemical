// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class FunctionTypeCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    FunctionTypeCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitFunctionType(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunctionType;
    }

    std::string compound_type_string() const override {
        return "FunctionTypeCST";
    }

};