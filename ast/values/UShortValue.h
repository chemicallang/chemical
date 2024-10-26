// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UShortType.h"

class UShortValue : public IntNumValue {
public:

    unsigned short value;
    SourceLocation location;

    explicit UShortValue(unsigned short value, SourceLocation location) : value(value), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::UShort;
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &UShortType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &UShortType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    UShortValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UShortValue>()) UShortValue(value, location);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UShortType>()) UShortType(location);
    }

    unsigned int get_num_bits() final {
        return 16;
    }

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    int64_t get_num_value() const final {
        return value;
    }

    [[nodiscard]] ValueType value_type() const final {
        return ValueType::UShort;
    }

};