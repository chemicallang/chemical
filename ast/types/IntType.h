// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class IntType : public IntNType {
public:

    IntType() : IntNType(32, false) {

    }

    ValueType value_type() const override {
        return ValueType::Int;
    }

    BaseType *copy() const override {
        return new IntType();
    }

};