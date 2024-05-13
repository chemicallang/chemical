// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class CastCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    CastCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitCast(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompCastValue;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "CastCST";
    }

#endif

};