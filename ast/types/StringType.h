// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public BaseType {
public:

    [[nodiscard]]
    std::unique_ptr<BaseType> create_child_type() const override;

    hybrid_ptr<BaseType> get_child_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::String;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::String;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::String;
    }

    bool satisfies(Value *value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == kind();
    }

    [[nodiscard]]
    StringType *copy() const override {
        return new StringType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};