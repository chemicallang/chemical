// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class AddrOfCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    AddrOfCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitAddrOf(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompAddrOf;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "AddrOfCST";
    }

#endif

};