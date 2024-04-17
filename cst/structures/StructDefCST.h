// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class StructDefCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    StructDefCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompStructDef;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "StructDefCST";
    }

#endif

};