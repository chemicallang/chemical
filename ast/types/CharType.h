// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNType.h"

class CharType : public IntNType {
public:

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    unsigned int num_bits() const override {
        return 8;
    }

    bool is_unsigned() override {
        return false;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    bool satisfies(ValueType type) const override {
        return type == ValueType::Char;
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Char;
    }

    ValueType value_type() const override {
        return ValueType::Char;
    }

    bool satisfies(Value *value) override;

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new CharType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) override;
#endif

};