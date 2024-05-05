// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UShortType : public IntNType {
public:

    UShortType() : IntNType(16, true) {

    }

    ValueType value_type() const override {
        return ValueType::UShort;
    }

    BaseType *copy() const override {
        return new UShortType();
    }

};