// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class IntNType : public TokenizedBaseType {
public:

    using TokenizedBaseType::TokenizedBaseType;

    /**
     * is this int n type unsigned
     */
    virtual bool is_unsigned() = 0;

    /**
     * the number of bits, int means int32 which has 32 bits
     */
    [[nodiscard]]
    virtual unsigned int num_bits() const = 0;

    /**
     * helper
     */
    inline unsigned int number() const {
        return num_bits();
    }

    /**
     * creates a Value of this type, but with the given value
     */
    virtual Value* create(int64_t value) = 0;

    bool satisfies(ValueType type) override {
        return type == value_type();
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::IntN;
    }

    bool is_same(BaseType *type) override {
        return type->kind() == kind() && ((IntNType*) type)->num_bits() == num_bits() && ((IntNType*) type)->is_unsigned() == is_unsigned();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};