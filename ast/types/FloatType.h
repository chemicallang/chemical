// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class FloatType : public BaseType {
public:

    /**
     * constructor
     */
    constexpr FloatType() : BaseType(BaseTypeKind::Float) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Float;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    FloatType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<FloatType>()) FloatType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};