// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UIntType : public IntNType {
public:

    UIntType() : IntNType(32, true) {

    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::UInt;
    }

    BaseType *copy() const override {
        return new UIntType();
    }

    std::string representation() const override {
        return "uint";
    }

};