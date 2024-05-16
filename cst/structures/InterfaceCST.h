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
        visitor->visitInterface(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompInterface;
    }

};