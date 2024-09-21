// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class ShortType : public IntNType {
public:

    static const ShortType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const override {
        return 16;
    }

    bool is_unsigned() override {
        return false;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 2;
    }

    bool satisfies(ASTAllocator& allocator, Value* value) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Short;
    }

    [[nodiscard]]
    ShortType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<ShortType>()) ShortType(token);
    }

};