// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class Int128Type : public IntNType {
public:

    static const Int128Type instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 128;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(int64_t value) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Int128;
    }

    [[nodiscard]]
    Int128Type *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<Int128Type>()) Int128Type(location);
    }

};