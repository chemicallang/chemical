// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public BaseType {
public:

    /**
     * constructor
     */
    constexpr BoolType() : BaseType(BaseTypeKind::Bool) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    bool satisfies(BaseType *type) final;

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Bool;
    }

    BoolType* copy(ASTAllocator& allocator) final {
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