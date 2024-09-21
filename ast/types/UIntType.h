// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UIntType : public IntNType {
public:

    static const UIntType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const override {
        return 32;
    }

    bool is_unsigned() override {
        return true;
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
        return ValueType::UInt;
    }

    [[nodiscard]]
    UIntType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<UIntType>()) UIntType(token);
    }

};