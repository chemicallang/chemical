// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class ComplexType : public TokenizedBaseType {
public:

    BaseType* elem_type;

    ComplexType(BaseType* elem_type, CSTToken* token) : TokenizedBaseType(token), elem_type(elem_type) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Float;
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Complex;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Float;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Complex;
    }

    bool can_promote(Value *value) final;

    Value *promote(Value *value) final;

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Complex;
    }

    [[nodiscard]]
    ComplexType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ComplexType>()) ComplexType(elem_type, token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};