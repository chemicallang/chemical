// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UCharType.h"

class UCharValue : public IntNumValue {
public:

    unsigned char value;
    CSTToken* token;

    explicit UCharValue(unsigned char value, CSTToken* token) : value(value), token(token) {

    }

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::UChar;
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &UCharType::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &UCharType::instance;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UCharValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<UCharValue>()) UCharValue(value, token);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) override {
        return new (allocator.allocate<UCharType>()) UCharType(nullptr);
    }

    bool is_unsigned() override {
        return true;
    }

    unsigned int get_num_bits() override {
        return 8;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UChar;
    }

};