// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class VarInitCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    VarInitCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    bool is_var_init() override {
        return true;
    }

    /**
     * returns if this has const keyword instead of var in the first token
     */
    bool is_const();

    /**
     * get the identifier, the name of the variable
     */
    std::string identifier();

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "VarInitCST";
    }

#endif

};