// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class DereferenceCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    DereferenceCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitDereference(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompDeference;
    }

};