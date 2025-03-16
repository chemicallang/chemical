// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/LongType.h"

class LongValue : public IntNumValue {
public:

    long value;

    /**
     * constructor
     */
    constexpr LongValue(
        long value,
        SourceLocation location
    ) : IntNumValue(ValueKind::Long, location), value(value) {

    }

    BaseType* known_type() final {
        return (BaseType*) &LongType::instance;
    }

    uint64_t byte_size(bool is64Bit_) final {
        return is64Bit_ ? 8 : 4;
    }

    LongValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<LongValue>()) LongValue(value, encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<LongType>()) LongType(encoded_location());
    }

    unsigned int get_num_bits(bool is64Bit) final {
        if(is64Bit) {
            return 64;
        } else {
            return 32;
        }
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]] uint64_t get_num_value() const final {
        return value;
    }

};