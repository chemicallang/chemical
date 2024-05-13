// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class StructValueCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    StructValueCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitStructValue(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompStructValue;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "StructValueCST";
    }

#endif

};