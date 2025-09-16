// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public BaseType {
public:

    /**
     * constructor
     */
    constexpr StringType() : BaseType(BaseTypeKind::String) {

    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final;

    BaseType* known_child_type() final;

    bool satisfies(BaseType *type) final;

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    StringType *copy(ASTAllocator& allocator) final {
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