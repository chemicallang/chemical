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

    unsigned int precedence() override {
        return 1;
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Float;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    std::unique_ptr<Value> promote(Value* value) override;

    virtual BaseType* copy() const {
        return new FloatType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};