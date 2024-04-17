// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class DoWhileCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    DoWhileCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompDoWhile;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "DoWhileCST";
    }

#endif

};