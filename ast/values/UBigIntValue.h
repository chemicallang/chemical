// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UBigIntType.h"

class UBigIntValue : public IntNumValue {
public:

    unsigned long long value;
    CSTToken* token;

    explicit UBigIntValue(unsigned long long value, CSTToken* token) : value(value), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::UBigInt;
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &UBigIntType::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &UBigIntType::instance;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UBigIntValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<UBigIntValue>()) UBigIntValue(value, token);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator& allocator) override {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(token);
    }

    unsigned int get_num_bits() override {
        return 64;
    }

    bool is_unsigned() override {
        return true;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UBigInt;
    }

};