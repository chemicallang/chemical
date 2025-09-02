// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"

class BigIntValue : public IntNumValue {
public:

    long long value;

    /**
     * constructor
     */
    constexpr BigIntValue(
            long long value,
            BigIntType* type,
            SourceLocation location
    ) : IntNumValue(ValueKind::BigInt, type, location), value(value) {

    }

    BigIntType* getType() {
        return (BigIntType*) IntNumValue::getType();
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    BigIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<BigIntValue>()) BigIntValue(value, getType(), encoded_location());
    }

    unsigned int get_num_bits(bool is64Bit) final {
        return 64;
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

};