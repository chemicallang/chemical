// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UCharType.h"

class UCharValue : public IntNumValue {
public:

    unsigned char value;

    /**
     * constructor
     */
    constexpr UCharValue(
        unsigned char value,
        UCharType* ucharType,
        SourceLocation location
    ) : IntNumValue(ValueKind::UChar, ucharType, location), value(value) {

    }

    inline UCharType* getType() const noexcept {
        return (UCharType*) IntNumValue::getType();
    }

    BaseType* known_type() final {
        return (BaseType*) &UCharType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    UCharValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UCharValue>()) UCharValue(value, getType(), encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UCharType>()) UCharType();
    }

    bool is_unsigned() final {
        return true;
    }

    unsigned int get_num_bits(bool is64Bit) final {
        return 8;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

};