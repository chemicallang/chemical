// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class FloatType : public TokenizedBaseType {
public:

    static const FloatType instance;

    using TokenizedBaseType::TokenizedBaseType;

    uint64_t byte_size(bool is64Bit) override {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::Float;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Float;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Float;
    }

    bool satisfies(BaseType *type) override {
        return type->kind() == BaseTypeKind::Float;
    }

    bool can_promote(Value *value) override;

    Value *promote(Value *value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == kind();
    }

    [[nodiscard]]
    FloatType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<FloatType>()) FloatType(token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    clang::QualType clang_type(clang::ASTContext &context) override;

#endif

};