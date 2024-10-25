// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public TokenizedBaseType {
public:

    static const BoolType instance;

    using TokenizedBaseType::TokenizedBaseType;

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Bool;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Bool;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Bool;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Bool;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Bool;
    }

    virtual BoolType* copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<BoolType>()) BoolType(token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};