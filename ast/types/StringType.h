// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public BaseType {
public:

    static const StringType instance;

    /**
     * constructor
     */
    constexpr StringType(SourceLocation location) : BaseType(BaseTypeKind::String, location) {

    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final;

//    hybrid_ptr<BaseType> get_child_type() final;

    BaseType* known_child_type() final;

    bool satisfies(BaseType *type) final;

    bool is_same(BaseType *type) final {
        return type->kind() == kind();
    }

    [[nodiscard]]
    StringType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<StringType>()) StringType(encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

#endif

};