// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class InterfaceCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    InterfaceCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompInterface;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "InterfaceCST";
    }

#endif

};