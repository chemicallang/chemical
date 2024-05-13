// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ImplCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ImplCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitImpl(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompImpl;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "ImplCST";
    }

#endif

};