// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <utility>

#include "ast/base/BaseType.h"
#include "std/chem_string_view.h"

class LinkedType : public BaseType {
public:

    chem::string_view type;
    ASTNode *linked;

    constexpr LinkedType(chem::string_view type, SourceLocation location) : type(type), BaseType(BaseTypeKind::Linked, location), linked(nullptr) {

    }

    [[deprecated]]
    constexpr LinkedType(chem::string_view type, SourceLocation location, ASTNode* linked) : type(type), BaseType(BaseTypeKind::Linked, location), linked(linked) {

    }

    constexpr LinkedType(chem::string_view type, ASTNode* linked, SourceLocation location) : type(type), BaseType(BaseTypeKind::Linked, location), linked(linked) {

    }

    uint64_t byte_size(bool is64Bit) final;

    bool link(SymbolResolver &linker);

    ASTNode *linked_node() final;

    bool is_same(BaseType *other) final {
        return other->kind() == kind() && static_cast<LinkedType *>(other)->linked == linked;
    }

    bool satisfies(BaseType *type) final;

    [[nodiscard]]
    LinkedType *copy(ASTAllocator& allocator) const {
        return new (allocator.allocate<LinkedType>()) LinkedType(type, linked, encoded_location());
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_param_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

#endif

};