// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UBigIntType.h"

class UBigIntValue : public IntNumValue {
public:

    unsigned long long value;

    /**
     * constructor
     */
    constexpr UBigIntValue(
        unsigned long long value,
        SourceLocation location
    ) : IntNumValue(ValueKind::UBigInt, location), value(value) {

    }


//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &UBigIntType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &UBigIntType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    UBigIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value, encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator& allocator) final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(encoded_location());
    }

    unsigned int get_num_bits(bool is64Bit) final {
        return 64;
    }

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

};