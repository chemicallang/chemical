// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"

class BigIntValue : public IntNumValue {
public:

    long long value;

    explicit BigIntValue(
        long long value,
        SourceLocation location
    ) : IntNumValue(ValueKind::BigInt, location), value(value) {

    }


    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    BigIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<BigIntValue>()) BigIntValue(value, encoded_location());
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &BigIntType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &BigIntType::instance;
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<BigIntType>()) BigIntType(encoded_location());
    }

    unsigned int get_num_bits() final {
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