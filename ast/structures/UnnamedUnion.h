// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/UnionType.h"
#include "ast/types/LinkedType.h"

class UnnamedUnion : public BaseDefMember, public VariablesContainer {
public:

    AccessSpecifier specifier;
    LinkedType linkedType;

    /**
     * constructor
     */
    UnnamedUnion(
        chem::string_view name,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : BaseDefMember(name, ASTNodeKind::UnnamedUnion, parent_node, location), specifier(specifier), linkedType("", this, location) {

    }


//    VariablesContainer *variables_container() final {
//        return this;
//    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

    bool get_is_const() final {
        // TODO allow user to mark unnamed struct's const
        return false;
    }

    UnnamedUnion* copy_member(ASTAllocator &allocator) final {
        auto unnamed = new (allocator.allocate<UnnamedUnion>()) UnnamedUnion(name, parent(), encoded_location());
        VariablesContainer::copy_into(*unnamed, allocator, this);
        return unnamed;
    }

    void link_signature(SymbolResolver &linker) override;

    ASTNode *child(const chem::string_view &name) final {
        return VariablesContainer::child_def_member(name);
    }

    uint64_t byte_size(bool is64Bit) final {
        return largest_member()->byte_size(is64Bit);
    }

    BaseType* create_value_type(ASTAllocator &allocator) final;

    BaseType* known_type() override {
        return &linkedType;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &values, unsigned int index) final;

    bool add_child_index(
        Codegen &gen,
        std::vector<llvm::Value *> &indexes,
        const chem::string_view &name
    ) final {
        return llvm_union_child_index(gen, indexes, name);
    }

#endif

};