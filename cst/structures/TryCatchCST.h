// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class TryCatchCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    TryCatchCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitTryCatch(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompTryCatch;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "TryCatchCST";
    }

#endif

};