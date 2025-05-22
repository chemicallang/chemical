// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>
#include "ast/base/TypeLoc.h"
#include "ast/types/LinkedType.h"
#include <unordered_map>

class ASTDiagnoser;

class GenericType : public BaseType {
public:

    LinkedType* referenced;
    std::vector<TypeLoc> types;

    /**
     * constructor
     */
    GenericType(LinkedType* referenced) : BaseType(BaseTypeKind::Generic), referenced(referenced) {

    }

    /**
     * constructor
     */
    GenericType(LinkedType* referenced, std::vector<TypeLoc> types) : BaseType(BaseTypeKind::Generic), referenced(referenced), types(std::move(types)) {

    }

    /**
     * link func
     */
    bool link(SymbolResolver &linker, SourceLocation loc) final;

    ASTNode *linked_node() final;

    bool is_same(BaseType *other) final {
        return other->kind() == kind();
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    GenericType* copy(ASTAllocator& allocator) const final;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

#endif

};