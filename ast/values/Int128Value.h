// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"
#include "ast/types/Int128Type.h"

class Int128Value : public IntNumValue {
public:

    uint64_t magnitude;
    bool is_negative;
    SourceLocation location;

    Int128Value(uint64_t magnitude, bool is_negative, SourceLocation location) : magnitude(magnitude), is_negative(is_negative), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::Int128;
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &Int128Type::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &Int128Type::instance;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    Int128Value *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<Int128Value>()) Int128Value(magnitude, is_negative, location);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<Int128Type>()) Int128Type(location);
    }

    unsigned int get_num_bits() final {
        return 128;
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
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

    [[nodiscard]] ValueType value_type() const final {
        return ValueType::Int128;
    }

};