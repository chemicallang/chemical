// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UShortType.h"

class UShortValue : public IntNumValue {
public:

    unsigned short value;

    /**
     * constructor
     */
    constexpr UShortValue(
        unsigned short value,
        SourceLocation location
    ) : IntNumValue(ValueKind::UShort, location), value(value) {

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

    UShortValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UShortValue>()) UShortValue(value, encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UShortType>()) UShortType(encoded_location());
    }

    unsigned int get_num_bits() final {
        return 16;
    }

    bool is_unsigned() final {
        return true;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

};