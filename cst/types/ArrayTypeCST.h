// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class ArrayTypeCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    ArrayTypeCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    std::string compound_type_string() const override {
        return "ArrayType";
    }

};