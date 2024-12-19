// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UShortType : public IntNType {
public:

    static const UShortType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 16;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(ASTAllocator& allocator, int64_t value) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::UShort;
    }

    [[nodiscard]]
    UShortType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UShortType>()) UShortType(location);
    }

};