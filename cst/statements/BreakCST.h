// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class BreakCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    BreakCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitBreak(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompBreak;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "BreakCST";
    }

#endif

};