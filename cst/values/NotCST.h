// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class NotCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    NotCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitNot(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompNot;
    }

};