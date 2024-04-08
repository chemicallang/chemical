// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class FunctionCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    FunctionCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "FunctionCST";
    }

#endif

};