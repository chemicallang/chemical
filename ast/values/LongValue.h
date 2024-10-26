// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/LongType.h"

class LongValue : public IntNumValue {
public:

    long value;
    bool is64Bit;
    SourceLocation location;

    LongValue(long value, bool is64Bit, SourceLocation location) : value(value), is64Bit(is64Bit), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::Long;
    }

//    hybrid_ptr<BaseType> get_base_type() final {
//        return hybrid_ptr<BaseType> { known_type(), false };
//    }

    BaseType* known_type() final {
        return (BaseType*) (is64Bit ? &LongType::instance64Bit : &LongType::instance32Bit);
    }

    uint64_t byte_size(bool is64Bit_) final {
        return is64Bit_ ? 8 : 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    LongValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<LongValue>()) LongValue(value, is64Bit, location);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<LongType>()) LongType(is64Bit, location);
    }

    unsigned int get_num_bits() final {
        if(is64Bit) {
            return 64;
        } else {
            return 32;
        }
    }

    bool is_unsigned() final {
        return false;
    }

    [[nodiscard]] int64_t get_num_value() const final {
        return value;
    }

    [[nodiscard]] ValueType value_type() const final {
        return ValueType::Long;
    }

};