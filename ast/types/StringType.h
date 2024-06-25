// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public BaseType {
public:

    std::unique_ptr<BaseType> create_child_type() const override;

    hybrid_ptr<BaseType> get_child_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) const override {
        return type == ValueType::String;
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::String;
    }

    ValueType value_type() const override {
        return ValueType::String;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType *copy() const {
        return new StringType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) const override;

#endif

};