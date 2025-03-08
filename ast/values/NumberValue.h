// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"
#include "ast/types/IntType.h"
#include "ast/types/LongType.h"
#include "ast/base/Value.h"

/**
 * @brief Class representing a number value integer / long
 *
 * suppose if user writes a function that returns long, parsing a int32 value would lose information
 * we could parse every value as a bigint value and return as appropriate type
 *
 * but it must link with the type it must return as, so this value is used, which automatically links
 * and returns as appropriate type automatically
 */
class NumberValue : public IntNumValue {
public:

    uint64_t value;

    /**
     * constructor
     */
    constexpr NumberValue(
        uint64_t value,
        SourceLocation location
    ) : IntNumValue(ValueKind::NumberValue, location), value(value) {}

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) final {
        return true;
    }

    unsigned int get_num_bits() final  {
        if(value > INT_MAX) {
            return 64;
        } else {
            return 32;
        }
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

    NumberValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NumberValue>()) NumberValue(value, encoded_location());
    }

    bool is_unsigned() final {
        return false;
    }

    BaseType* known_type() final {
        if(value > INT_MAX) {
            if(sizeof(void*) > 4) {
                return (BaseType*) &LongType::instance64Bit;
            } else {
                return (BaseType*) &LongType::instance32Bit;
            }
        } else {
            return (BaseType*) &IntType::instance;
        }
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        if(value > INT_MAX) {
            return new (allocator.allocate<LongType>()) LongType(sizeof(void*) > 4, encoded_location());
        } else {
            return new (allocator.allocate<IntType>()) IntType(encoded_location());
        }
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const final {
        return BaseTypeKind::IntN;
    }

};