// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public BaseType {
public:

    /**
     * constructor
     */
    constexpr DoubleType() : BaseType(BaseTypeKind::Double) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }

    [[nodiscard]]
    DoubleType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<DoubleType>()) DoubleType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};