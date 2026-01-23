// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "VariablesContainer.h"
#include "BaseDefMember.h"
#include "ast/types/StructType.h"

class UnnamedStruct : public BaseDefMember, public VariablesContainer {
public:

    AccessSpecifier specifier;
    LinkedType linkedType;

    /**
     * constructor
     */
    UnnamedStruct(
        chem::string_view name,
        ASTNode* parent_node,
        SourceLocation location,
        AccessSpecifier specifier = AccessSpecifier::Internal
    ) : BaseDefMember(name, ASTNodeKind::UnnamedStruct, parent_node, location), specifier(specifier),
        linkedType(this)
    {

    }

    VariablesContainer *as_variables_container() final {
        return this;
    }

    bool get_is_const() final {
        // TODO allow user to mark unnamed structs const
        return false;
    }

    UnnamedStruct* copy_member(ASTAllocator &allocator) final {
        const auto unnamed = new (allocator.allocate<UnnamedStruct>()) UnnamedStruct(name, parent(), encoded_location());
        VariablesContainer::copy_into(*unnamed, allocator, this);
        return unnamed;
    }

    bool requires_copy_fn() {
        for(const auto var : variables()) {
            if(var->known_type()->requires_copy_fn()) {
                return true;
            }
        }
        return false;
    }

    bool requires_destructor() {
        for(const auto var : variables()) {
            if(var->known_type()->requires_destructor()) {
                return true;
            }
        }
        return false;
    }

    uint64_t byte_size(TargetData& target) final {
        return total_byte_size(target);
    }

    inline BaseType* known_type() {
        return &linkedType;
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<Value*> &values, unsigned int index) final;

    bool add_child_index(
            Codegen &gen,
            std::vector<llvm::Value *> &indexes,
            const chem::string_view &name
    ) final {
        return VariablesContainer::llvm_struct_child_index(gen, indexes, name);
    }

#endif

};