// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class CharType : public BaseType {
public:

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) const override {
        return type == ValueType::Char;
    }

    std::string representation() const override {
        return "char";
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Char;
    }

    ValueType value_type() const override {
        return ValueType::Char;
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