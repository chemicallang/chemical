// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class EnumDeclCST : public CompoundCSTToken {
public:

    /**
     * constructor
     */
    EnumDeclCST(std::vector<std::unique_ptr<CSTToken>> tokens) : CompoundCSTToken(std::move(tokens)) {

    }

    void accept(CSTVisitor *visitor) override {
        visitor->visitEnumDecl(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompEnumDecl;
    }

#ifdef DEBUG

    std::string compound_type_string() const override {
        return "EnumDeclCST";
    }

#endif

};