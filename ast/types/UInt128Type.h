// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UInt128Type : public IntNType {
public:

    UInt128Type() : IntNType(128, true) {

    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::UInt128;
    }

    BaseType *copy() const override {
        return new UInt128Type();
    }

    std::string representation() const override {
        return "__uint128";
    }

};