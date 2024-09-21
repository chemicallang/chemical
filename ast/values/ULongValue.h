// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/ULongType.h"

class ULongValue : public IntNumValue {
public:

    unsigned long value;
    bool is64Bit;
    CSTToken* token;

    ULongValue(unsigned long value, bool is64Bit, CSTToken* token) : value(value), is64Bit(is64Bit), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::ULong;
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { known_type(), false };
//    }

    BaseType* known_type() override {
        return (BaseType*) (is64Bit ? &ULongType::instance64Bit : &ULongType::instance32Bit);
    }

    uint64_t byte_size(bool is64Bit_) override {
        return is64Bit_ ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ULongValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<ULongValue>()) ULongValue(value, is64Bit, token);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) override {
        return new (allocator.allocate<ULongType>()) ULongType(is64Bit, nullptr);
    }

    unsigned int get_num_bits() override {
        if(is64Bit) {
            return 64;
        } else {
            return 32;
        }
    }

    bool is_unsigned() override {
        return true;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::ULong;
    }

};