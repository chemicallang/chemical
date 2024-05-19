// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class ShortType : public IntNType {
public:

    ShortType() : IntNType(16, false) {

    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::Short;
    }

    BaseType *copy() const override {
        return new ShortType();
    }

    std::string representation() const override {
        return "short";
    }

};