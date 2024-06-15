// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class IntNType : public BaseType {
public:

    /**
     * this is number of bits of the type, not the actual number
     * int means int32, meaning 32 bit integer
     */
    unsigned int number;
    /**
     * is the type unsigned
     */
    bool is_unsigned;

    /**
     * constructor
     */
    IntNType(unsigned int number, bool is_unsigned = false) : number(number), is_unsigned(is_unsigned) {
        // do nothing
    }

    /**
     * creates a Value of this type, but with the given value
     */
    virtual Value* create(int64_t value) = 0;

    bool satisfies(ValueType type) const override {
        return type == ValueType::Int;
    }

    ValueType value_type() const override {
        return ValueType::Int;
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::IntN;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) const override;

#endif

};