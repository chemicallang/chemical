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
        SourceLocation location
    ) : IntNumValue(ValueKind::UInt, location), value(value) {

    }


//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &UIntType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &UIntType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    UIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UIntValue>()) UIntValue(value, encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UIntType>()) UIntType(encoded_location());
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