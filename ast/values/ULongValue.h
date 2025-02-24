// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/ULongType.h"

class ULongValue : public IntNumValue {
public:

    unsigned long value;
    bool is64Bit;

    /**
     * constructor
     */
    ULongValue(
        unsigned long value,
        bool is64Bit,
        SourceLocation location
    ) : IntNumValue(ValueKind::UInt, location), value(value), is64Bit(is64Bit) {

    }


//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { known_type(), false };
//    }

    BaseType* known_type() final {
        return (BaseType*) (is64Bit ? &ULongType::instance64Bit : &ULongType::instance32Bit);
    }

    uint64_t byte_size(bool is64Bit_) final {
        return is64Bit_ ? 8 : 4;
    }

    ULongValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<ULongValue>()) ULongValue(value, is64Bit, encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<ULongType>()) ULongType(is64Bit, encoded_location());
    }

    unsigned int get_num_bits() final {
        if(is64Bit) {
            return 64;
        } else {
            return 32;
        }
    }

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

};