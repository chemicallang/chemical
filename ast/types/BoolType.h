// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Bool;
    }

    std::string representation() const override {
        return "bool";
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Bool;
    }

    ValueType value_type() const override {
        return ValueType::Bool;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new BoolType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};