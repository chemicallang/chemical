// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class WhileCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    WhileCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitWhile(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompWhile;
    }

};