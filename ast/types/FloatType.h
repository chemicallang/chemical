// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class FloatType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Float;
    }

    std::string representation() const override {
        return "float";
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Float;
    }

    ValueType value_type() const override {
        return ValueType::Float;
    }

    bool can_promote(Value *value) override;

    Value *promote(Value *value) override;

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType *copy() const {
        return new FloatType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) const override;

#endif

};