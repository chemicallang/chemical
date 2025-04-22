// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class LongDoubleType : public BaseType {
public:

    static const LongDoubleType instance;

    /**
     * constructor
     */
    constexpr LongDoubleType() : BaseType(BaseTypeKind::LongDouble) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::LongDouble;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    LongDoubleType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<LongDoubleType>()) LongDoubleType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};