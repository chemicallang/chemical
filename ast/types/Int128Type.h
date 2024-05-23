// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class Int128Type : public IntNType {
public:

    Int128Type() : IntNType(128, false) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return 16;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::Int128;
    }

    BaseType *copy() const override {
        return new Int128Type();
    }

    std::string representation() const override {
        return "__int128";
    }

};