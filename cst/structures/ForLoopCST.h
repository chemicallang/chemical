// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ForLoopCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ForLoopCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitForLoop(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompForLoop;
    }

};