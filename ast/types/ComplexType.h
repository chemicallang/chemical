// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class ComplexType : public BaseType {
public:

    BaseType* elem_type;

    /**
     * constructor
     */
    constexpr ComplexType(
        BaseType* elem_type
    ) : BaseType(BaseTypeKind::Complex), elem_type(elem_type) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Complex;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Complex;
    }

    [[nodiscard]]
    ComplexType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ComplexType>()) ComplexType(elem_type);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};