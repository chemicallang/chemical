// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ExpressionCST : public CompoundCSTToken {
public:

    unsigned int op_index;

    /**
     * constructor
     */
    ExpressionCST(std::vector<std::unique_ptr<CSTToken>> tokens, unsigned int op_index) : CompoundCSTToken(std::move(tokens)), op_index(op_index) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "ExpressionCST";
    }

#endif

};