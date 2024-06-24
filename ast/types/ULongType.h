// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class ULongType : public IntNType {
public:

    ULongType(bool is64Bit) : IntNType(is64Bit ? 64 : 32, true) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::ULong;
    }

    BaseType *copy() const override {
        return new ULongType(number == 64);
    }

};