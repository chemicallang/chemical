// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class ComplexType : public BaseType {
public:

    BaseType* elem_type;

    ComplexType(
        BaseType* elem_type,
        SourceLocation location
    ) : BaseType(BaseTypeKind::Complex, location), elem_type(elem_type) {

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
        return new (allocator.allocate<ComplexType>()) ComplexType(elem_type, encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};