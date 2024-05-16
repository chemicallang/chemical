// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class AssignmentCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    AssignmentCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitAssignment(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompAssignment;
    }

};