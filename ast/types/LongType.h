// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class LongType : public IntNType {
public:

    LongType(bool is64Bit) : IntNType(is64Bit ? 64 : 32, false) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::Long;
    }

    BaseType *copy() const override {
        return new LongType(number == 64);
    }

    std::string representation() const override {
        return "long";
    }


};