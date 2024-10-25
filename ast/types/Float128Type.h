// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class Float128Type : public TokenizedBaseType {
public:

    static const Float128Type instance;

    using TokenizedBaseType::TokenizedBaseType;

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(ValueType type) final {
        return type == ValueType::Float;
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Float128;
    }

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::Float;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Float128;
    }

    bool can_promote(Value *value) final;

    Value *promote(Value *value) final;

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    Float128Type *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<Float128Type>()) Float128Type(location);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};