// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"
#include "ast/types/Int128Type.h"

class Int128Value : public IntNumValue {
public:

    uint64_t magnitude;
    bool is_negative;

    Int128Value(
        uint64_t magnitude,
        bool is_negative,
        SourceLocation location
    ) : IntNumValue(ValueKind::Int128, location), magnitude(magnitude), is_negative(is_negative) {

    }


//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &Int128Type::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &Int128Type::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    Int128Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<Int128Value>()) Int128Value(magnitude, is_negative, encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<Int128Type>()) Int128Type(encoded_location());
    }

    unsigned int get_num_bits() final {
        return 128;
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        if(magnitude < UINT_MAX) {
            if(is_negative) {
                return -magnitude;
            } else {
                return magnitude;
            }
        } else {
            if(is_negative) {
                // Overflow: The Int128 value is too large to fit into uint64_t
                throw std::overflow_error("Int128 value exceeds uint64_t range");
            } else {
                return magnitude;
            }
        }
    }

};