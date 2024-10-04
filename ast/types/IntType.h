// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class IntType : public IntNType {
public:

    static const IntType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const override {
        return 32;
    }

    bool is_unsigned() override {
        return false;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Int;
    }

    [[nodiscard]]
    IntType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<IntType>()) IntType(token);
    }

};