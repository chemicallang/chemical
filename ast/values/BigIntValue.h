// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"

class BigIntValue : public IntNumValue {
public:

    long long value;
    SourceLocation location;

    explicit BigIntValue(long long value, SourceLocation location) : value(value), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::BigInt;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    BigIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<BigIntValue>()) BigIntValue(value, location);
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &BigIntType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &BigIntType::instance;
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<BigIntType>()) BigIntType(location);
    }

    unsigned int get_num_bits() final {
        return 64;
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
        return value;
    }

    [[nodiscard]] ValueType value_type() const final {
        return ValueType::BigInt;
    }

};