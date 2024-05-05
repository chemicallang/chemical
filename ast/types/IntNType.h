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

    IntNType(unsigned int number, bool is_unsigned = false) : number(number), is_unsigned(is_unsigned) {
        // do nothing
    }

    bool satisfies(ValueType type) const override {
        return type == ValueType::Int;
    }

    ValueType value_type() const override {
        return ValueType::Int;
    }

    std::string representation() const override {
        return "int" + std::to_string(number);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::IntN;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new IntNType(number);
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};