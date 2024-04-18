// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class Int32Type : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Int;
    }

    std::string representation() const override {
        return "int32";
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Int32;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new Int32Type();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};