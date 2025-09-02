// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"
#include "ast/types/IntType.h"

/**
 * @brief Class representing an integer value.
 */
class IntValue : public IntNumValue {
public:

    int value; ///< The integer value.

    /**
     * constructor
     */
    constexpr IntValue(
        int value,
        IntType* type,
        SourceLocation location
    ) : IntNumValue(ValueKind::Int, type, location), value(value) {}

    IntType* getType() const noexcept {
        return (IntType*) Value::getType();
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    unsigned int get_num_bits(bool is64Bit) final {
        return 32;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

    IntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<IntValue>()) IntValue(value, getType(), encoded_location());
    }

    bool is_unsigned() final {
        return false;
    }

};