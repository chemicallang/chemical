// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class IncDecCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    IncDecCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitIncDec(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompIncDec;
    }

};