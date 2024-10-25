// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UIntType : public IntNType {
public:

    static const UIntType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 32;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(int64_t value) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::UInt;
    }

    [[nodiscard]]
    UIntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UIntType>()) UIntType(location);
    }

};