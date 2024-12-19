// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UBigIntType : public IntNType {
public:

    static const UBigIntType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 64;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(ASTAllocator& allocator, int64_t value) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::UBigInt;
    }

    [[nodiscard]]
    UBigIntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(location);
    }

};