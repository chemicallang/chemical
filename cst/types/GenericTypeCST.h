// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class GenericTypeCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    GenericTypeCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitGenericType(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompGenericType;
    }

    std::string compound_type_string() const override {
        return "GenericTypeCST";
    }

};