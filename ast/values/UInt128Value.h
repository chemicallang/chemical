// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"
#include "ast/types/UInt128Type.h"

class UInt128Value : public IntNumValue {
public:

    uint64_t low;
    uint64_t high;
    CSTToken* token;

    UInt128Value(uint64_t low, uint64_t high, CSTToken* token) : low(low), high(high), token(token) {

    }

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::UInt128;
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &UInt128Type::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &UInt128Type::instance;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 16;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UInt128Value *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<UInt128Value>()) UInt128Value(low, high, token);
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator &allocator) override {
        return new (allocator.allocate<UInt128Type>()) UInt128Type(nullptr);
    }

    unsigned int get_num_bits() override {
        return 128;
    }

    bool is_unsigned() override {
        return true;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        if (high > 0) {
            // Overflow: The UInt128 value is too large to fit into a uint64_t
            throw std::overflow_error("UInt128 value exceeds uint64_t range");
        }
        return low;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UInt128;
    }

};