// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UBigIntType.h"

class UBigIntValue : public IntNumValue {
public:

    unsigned long long value;
    SourceLocation location;

    explicit UBigIntValue(unsigned long long value, SourceLocation location) : value(value), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::UBigInt;
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

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    UBigIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value, location);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator& allocator) final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(location);
    }

    unsigned int get_num_bits() final {
        return 64;
    }

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
        return value;
    }

    [[nodiscard]] ValueType value_type() const final {
        return ValueType::UBigInt;
    }

};