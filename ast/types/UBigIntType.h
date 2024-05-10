// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UBigIntType : public IntNType {
public:

    UBigIntType() : IntNType(64, true) {

    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::UBigInt;
    }

    BaseType *copy() const override {
        return new UBigIntType();
    }

    std::string representation() const override {
        return "ubigint";
    }

};