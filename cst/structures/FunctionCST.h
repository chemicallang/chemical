// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class FunctionParamCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    FunctionParamCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunctionParam;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "FunctionParamCST";
    }

#endif

};

class FunctionCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    FunctionCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visit(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompFunction;
    }

    /**
     * get the function name for this function
     */
    std::string func_name();

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "FunctionCST";
    }

#endif

};