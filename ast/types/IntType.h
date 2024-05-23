// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class IntType : public IntNType {
public:

    IntType() : IntNType(32, false) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::Int;
    }

    BaseType *copy() const override {
        return new IntType();
    }

    std::string representation() const override {
        return "int";
    }

};