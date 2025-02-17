// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class ComplexType : public TokenizedBaseType {
public:

    BaseType* elem_type;

    ComplexType(BaseType* elem_type, SourceLocation location) : TokenizedBaseType(location), elem_type(elem_type) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    [[nodiscard]]
    BaseTypeKind kind() const final {
        return BaseTypeKind::Complex;
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Complex;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Complex;
    }

    [[nodiscard]]
    ComplexType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ComplexType>()) ComplexType(elem_type, location);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};