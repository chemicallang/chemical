// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UShortType : public IntNType {
public:

    UShortType() : IntNType(16, true) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return 2;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::UShort;
    }

    BaseType *copy() const override {
        return new UShortType();
    }

    std::string representation() const override {
        return "ushort";
    }

};