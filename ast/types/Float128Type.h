// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class Float128Type : public BaseType {
public:

    static const Float128Type instance;

    /**
     * constructor
     */
    constexpr Float128Type(SourceLocation location) : BaseType(BaseTypeKind::Float128, location) {

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
    Float128Type *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<Float128Type>()) Float128Type(encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};