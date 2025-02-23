// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"

class StringType : public BaseType {
public:

    static const StringType instance;

    /**
     * constructor
     */
    StringType(SourceLocation location) : BaseType(BaseTypeKind::String, location) {

    }

    [[nodiscard]]
    BaseType* create_child_type(ASTAllocator& allocator) const final;

//    hybrid_ptr<BaseType> get_child_type() final;

    BaseType* known_child_type() final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

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