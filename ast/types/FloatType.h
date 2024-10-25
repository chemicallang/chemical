// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class FloatType : public TokenizedBaseType {
public:

    static const FloatType instance;

    using TokenizedBaseType::TokenizedBaseType;

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Float;
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Float;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Float;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Float;
    }

    bool can_promote(Value *value) final;

    Value *promote(Value *value) final;

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    FloatType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<FloatType>()) FloatType(token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};