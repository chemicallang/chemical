// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class VoidType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Void;
    }

    std::string representation() const override {
        return "void";
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Void;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new VoidType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};