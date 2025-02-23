// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UCharType.h"

class UCharValue : public IntNumValue {
public:

    unsigned char value;

    /**
     * constructor
     */
    explicit UCharValue(
        unsigned char value,
        SourceLocation location
    ) : IntNumValue(ValueKind::UChar, location), value(value) {

    }


//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { (BaseType*) &UCharType::instance, false };
//    }

    BaseType* known_type() final {
        return (BaseType*) &UCharType::instance;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    UCharValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<UCharValue>()) UCharValue(value, encoded_location());
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<UCharType>()) UCharType(encoded_location());
    }

    bool is_unsigned() final {
        return true;
    }

    unsigned int get_num_bits() final {
        return 8;
    }

    [[nodiscard]]
    uint64_t get_num_value() const final {
        return value;
    }

};