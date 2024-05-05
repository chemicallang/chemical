// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UIntType : public IntNType {
public:

    UIntType() : IntNType(32, true) {

    }

    ValueType value_type() const override {
        return ValueType::UInt;
    }

    BaseType *copy() const override {
        return new UIntType();
    }

};