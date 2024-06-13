// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class AnnotationCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitAnnotation(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompAnnotation;
    }

};