// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ReturnCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ReturnCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitReturn(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompReturn;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "ReturnCST";
    }

#endif

};