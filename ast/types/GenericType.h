// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>
#include "ast/base/TypeLoc.h"
#include "ast/types/LinkedType.h"
#include <unordered_map>

class ASTDiagnoser;
class GenericInstantiatorAPI;

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
     * instantiate inline only instantiates generic type declaration
     */
    bool instantiate_inline(GenericInstantiatorAPI& instantiatorApi, SourceLocation loc);

    /**
     * instantiates the generic declaration associated with this type
     * and saves it into linked pointer
     * should only be called if referenced type and subtypes were resolved successfully
     * @return true if instantiated, false if error occurred
     */
    bool instantiate(GenericInstantiatorAPI& instantiatorApi, SourceLocation loc);

    ASTNode *linked_node() final;

    bool is_same(BaseType *other) final {
        return other->kind() == kind();
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    GenericType* copy(ASTAllocator& allocator) final;

    uint64_t byte_size(bool is64Bit) override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

#endif

};