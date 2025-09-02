// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UIntType.h"

class UIntValue : public IntNumValue {
public:

    unsigned int value;

    /**
     * constructor
     */
    constexpr UIntValue(
        unsigned int value,
        UIntType* type,
        SourceLocation location
    ) : IntNumValue(ValueKind::UInt, type, location), value(value) {

    }

    UIntType* getType() const noexcept {
        return (UIntType*) IntNumValue::getType();
    }

    BaseType* known_type() final {
        return (BaseType*) &UIntType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    UIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UIntValue>()) UIntValue(value, getType(), encoded_location());
    }

    bool is_unsigned() final {
        return true;
    }

    unsigned int get_num_bits(bool is64Bit) final {
        return 32;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

};