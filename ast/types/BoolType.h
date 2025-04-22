// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class BoolType : public BaseType {
public:

    static const BoolType instance;

    /**
     * constructor
     */
    constexpr BoolType() : BaseType(BaseTypeKind::Bool) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 1;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Bool;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Bool;
    }

    virtual BoolType* copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<BoolType>()) BoolType();
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};