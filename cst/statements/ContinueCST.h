// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ContinueCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ContinueCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitContinue(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompContinue;
    }

};