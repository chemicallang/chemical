// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class FloatType : public BaseType {
public:

    uint64_t byte_size(bool is64Bit) override {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::Float;
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Float;
    }

    ValueType value_type() const override {
        return ValueType::Float;
    }

    bool satisfies(Value *value) override;

    bool can_promote(Value *value) override;

    Value *promote(Value *value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == kind();
    }

    virtual BaseType *copy() const {
        return new FloatType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};