// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class BigIntType : public IntNType {
public:

    BigIntType() : IntNType(64, false) {

    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::BigInt;
    }

    BaseType *copy() const override {
        return new BigIntType();
    }

    std::string representation() const override {
        return "bigint";
    }

};