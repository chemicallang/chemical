// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class ShortType : public IntNType {
public:

    static const ShortType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 16;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(int64_t value) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Short;
    }

    [[nodiscard]]
    ShortType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ShortType>()) ShortType(location);
    }

};