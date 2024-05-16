// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class SwitchCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    SwitchCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitSwitch(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompSwitch;
    }

};