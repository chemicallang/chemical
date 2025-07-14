// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"
#include "ast/types/UInt128Type.h"

class UInt128Value : public IntNumValue {
public:

    uint64_t low;
    uint64_t high;

    /**
     * constructor
     */
    constexpr UInt128Value(
        uint64_t low,
        uint64_t high,
        UInt128Type* type,
        SourceLocation location
    ) : IntNumValue(ValueKind::UInt128, type, location), low(low), high(high) {

    }

    UInt128Type* getType() const noexcept {
        return (UInt128Type*) IntNumValue::getType();
    }

    BaseType* known_type() final {
        return (BaseType*) &UInt128Type::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    UInt128Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UInt128Value>()) UInt128Value(low, high, getType(), encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UInt128Type>()) UInt128Type();
    }

    unsigned int get_num_bits(bool is64Bit) final {
        return 128;
    }

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        if (high > 0) {
            // Overflow: The UInt128 value is too large to fit into a uint64_t
            throw std::overflow_error("UInt128 value exceeds uint64_t range");
        }
        return low;
    }

};