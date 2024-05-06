// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class ULongType : public IntNType {
public:

    ULongType(bool is64Bit) : IntNType(is64Bit ? 64 : 32, true) {

    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::ULong;
    }

    BaseType *copy() const override {
        return new ULongType(number == 64);
    }

};