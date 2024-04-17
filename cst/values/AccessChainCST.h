// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class AccessChainCST : public CompoundCSTToken {
public:

    /**
     * this means that Access chain is not nested as a value
     */
    bool is_node = false;

    /**
     * constructor
     */
    AccessChainCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    LexTokenType type() const override {
        return LexTokenType::CompAccessChain;
    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "AccessChainCST";
    }

#endif

};