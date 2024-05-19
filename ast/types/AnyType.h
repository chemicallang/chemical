// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class AnyType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return true;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    std::string representation() const override {
        return "any";
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Any;
    }

    bool is_same(BaseType *type) const override {
        return true;
    }

    virtual BaseType* copy() const {
        return new AnyType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};