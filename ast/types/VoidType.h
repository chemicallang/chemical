// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class VoidType : public BaseType {
public:

    static const VoidType instance;

    VoidType(SourceLocation location) : BaseType(BaseTypeKind::Void, location) {

    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Void;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    virtual VoidType* copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<VoidType>()) VoidType(encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};