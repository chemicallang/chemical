// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UIntType.h"

class UIntValue : public IntNumValue {
public:

    unsigned int value;
    SourceLocation location;

    explicit UIntValue(unsigned int value, SourceLocation location) : value(value), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::UInt;
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

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    UIntValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UIntValue>()) UIntValue(value, location);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UIntType>()) UIntType(location);
    }

    bool is_unsigned() final {
        return true;
    }

    unsigned int get_num_bits() final {
        return 32;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
        return value;
    }

    [[nodiscard]] ValueType value_type() const final {
        return ValueType::UInt;
    }

};