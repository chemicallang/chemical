// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class Float128Type : public BaseType {
public:

    static const Float128Type instance;

    /**
     * constructor
     */
    constexpr Float128Type() : BaseType(BaseTypeKind::Float128) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Float128;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    Float128Type *copy(ASTAllocator& allocator) final {
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