// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class AccessChainCST : public CompoundCSTToken {
public:

    /**
     * this means that Access chain is not nested as a value
     */
    bool is_node = false;

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitAccessChain(this);
    }

};