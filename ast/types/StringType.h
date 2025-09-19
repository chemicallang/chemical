// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public GlobalBaseType {
public:

    /**
     * constructor
     */
    constexpr StringType() : GlobalBaseType(BaseTypeKind::String) {

    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final;

    BaseType* known_child_type() final;

    bool satisfies(BaseType *type) final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};