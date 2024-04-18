// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class CharType : public BaseType {
public:

    bool satisfies(ValueType type) const override {
        return type == ValueType::Char;
    }

    std::string representation() const override {
        return "char";
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Char;
    }

    bool is_same(BaseType *type) const override {
        return type->kind() == kind();
    }

    virtual BaseType* copy() const {
        return new CharType();
    }

#ifdef COMPILER_BUILD
    llvm::Type *llvm_type(Codegen &gen) const override;
#endif

};