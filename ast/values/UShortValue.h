// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UShortType.h"

class UShortValue : public IntNumValue {
public:

    unsigned short value;
    CSTToken* token;

    explicit UShortValue(unsigned short value, CSTToken* token) : value(value), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::UShort;
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &UShortType::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &UShortType::instance;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 2;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UShortValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<UShortValue>()) UShortValue(value, token);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) override {
        return new (allocator.allocate<UShortType>()) UShortType(nullptr);
    }

    unsigned int get_num_bits() override {
        return 16;
    }

    bool is_unsigned() override {
        return true;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UShort;
    }

};