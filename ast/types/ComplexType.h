// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class ComplexType : public TokenizedBaseType {
public:

    BaseType* elem_type;

    ComplexType(BaseType* elem_type, CSTToken* token) : TokenizedBaseType(token), elem_type(elem_type) {

    }

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) override {
        return type == ValueType::Float;
    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Complex;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Float;
    }

    bool satisfies(BaseType *type) override {
        return type->kind() == BaseTypeKind::Complex;
    }

    bool can_promote(Value *value) override;

    Value *promote(Value *value) override;

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::Complex;
    }

    [[nodiscard]]
    ComplexType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<ComplexType>()) ComplexType(elem_type, token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    clang::QualType clang_type(clang::ASTContext &context) override;

#endif

};