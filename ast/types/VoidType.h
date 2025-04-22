// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class VoidType : public BaseType {
public:

    static const VoidType instance;

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
    virtual VoidType* copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<VoidType>()) VoidType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};