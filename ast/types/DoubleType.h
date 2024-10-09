// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public TokenizedBaseType {
public:

    static const DoubleType instance;

    using TokenizedBaseType::TokenizedBaseType;

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::Double;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Double;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Double;
    }

    bool satisfies(BaseType *type) override {
        return type->kind() == BaseTypeKind::Double;
    }

    bool can_promote(Value *value) override;

    Value *promote(Value *value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::Double;
    }

    [[nodiscard]]
    DoubleType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<DoubleType>()) DoubleType(token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    clang::QualType clang_type(clang::ASTContext &context) override;

#endif

};