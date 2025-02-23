// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class DoubleType : public BaseType {
public:

    static const DoubleType instance;

    /**
     * constructor
     */
    DoubleType(SourceLocation location) : BaseType(BaseTypeKind::Double, location) {

    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool satisfies(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }

    bool is_same(BaseType *type) final {
        return type->kind() == BaseTypeKind::Double;
    }

    [[nodiscard]]
    DoubleType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<DoubleType>()) DoubleType(encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    clang::QualType clang_type(clang::ASTContext &context) final;

#endif

};