// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CompoundCSTToken.h"

class GenericParamsListCST : public CompoundCSTToken {
public:

    using CompoundCSTToken::CompoundCSTToken;

    void accept(CSTVisitor *visitor) override {
        visitor->visitGenericParamsList(this);
    }

    LexTokenType type() const override {
        return LexTokenType::CompGenericParamsList;
    }

};