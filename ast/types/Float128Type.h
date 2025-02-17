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

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Float128;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Float128;
    }

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