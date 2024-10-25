// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public TokenizedBaseType {
public:

    static const DoubleType instance;

    using TokenizedBaseType::TokenizedBaseType;

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Double;
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Double;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Double;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }

    bool can_promote(Value *value) final;

    Value *promote(Value *value) final;

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }

    [[nodiscard]]
    DoubleType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<DoubleType>()) DoubleType(location);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};