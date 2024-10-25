// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class ULongType : public IntNType {
public:

    bool is64Bit;

    static const ULongType instance64Bit;
    static const ULongType instance32Bit;

    ULongType(bool is64Bit, CSTToken* token) : is64Bit(is64Bit), IntNType(token) {

    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return is64Bit ? 64 : 32;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(int64_t value) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::ULong;
    }

    [[nodiscard]]
    ULongType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ULongType>()) ULongType(is64Bit, token);
    }

};