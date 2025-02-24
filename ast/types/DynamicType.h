// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "LinkedType.h"

class DynamicType : public BaseType {
public:

    BaseType* referenced;

    /**
     * constructor
     */
    DynamicType(
        BaseType* referenced,
        SourceLocation location
    ) : referenced(referenced), BaseType(BaseTypeKind::Dynamic, location) {

    }

    bool is_same(BaseType* type) final {
        return type->kind() == BaseTypeKind::Dynamic && ((DynamicType*) type)->referenced->is_same(referenced);
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    DynamicType* copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<DynamicType>()) DynamicType(referenced->copy(allocator), encoded_location());
    }

    ASTNode* linked_node() final {
        return referenced->linked_node();
    }

    bool link(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen& gen) final;

    llvm::Type* llvm_param_type(Codegen& gen) final;

#endif

};