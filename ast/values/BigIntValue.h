// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"

class BigIntValue : public IntNumValue {
public:

    long long value;
    CSTToken* token;

    explicit BigIntValue(long long value, CSTToken* token) : value(value), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::BigInt;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BigIntValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<BigIntValue>()) BigIntValue(value, token);
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &BigIntType::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &BigIntType::instance;
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) override {
        return new (allocator.allocate<BigIntType>()) BigIntType(nullptr);
    }

    unsigned int get_num_bits() override {
        return 64;
    }

    bool is_unsigned() override {
        return false;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::BigInt;
    }

};