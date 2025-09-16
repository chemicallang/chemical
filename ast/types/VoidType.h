// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class VoidType : public BaseType {
public:

    /**
     * constructor
     */
    constexpr VoidType() : BaseType(BaseTypeKind::Void) {

    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Void;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    VoidType* copy(ASTAllocator& allocator) final {
        // why does this return itself (without copying)
        // because the type exists in type builder
        // it is initialized once in the type builder
        // this will never be copied
        return this;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};