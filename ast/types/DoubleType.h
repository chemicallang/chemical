// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Double;
    }

    std::string representation() const override {
        return "double";
    }

    unsigned int precedence() override {
        return 1;
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Double;
    }

    ValueType value_type() const override {
        return ValueType::Double;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new DoubleType();
    }

    std::unique_ptr<Value> promote(Value* value) override;

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};