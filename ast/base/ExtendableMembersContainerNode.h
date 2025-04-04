// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ExtendableBase.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/GenericFuncDecl.h"

class ExtendableMembersContainerNode : public MembersContainer, public ExtendableBase {
public:

    using MembersContainer::requires_moving;

    /**
     * the identifier of the container
     */
    LocatedIdentifier identifier;

    /**
     * constructor
     */
    ExtendableMembersContainerNode(
        LocatedIdentifier identifier,
        ASTNodeKind k,
        ASTNode* parent,
        SourceLocation location
    ) : MembersContainer(k, parent, location), identifier(identifier) {

    }

    /**
     * get the name of the container
     */
    inline const chem::string_view& name_view() const {
        return identifier.identifier;
    }

    /**
     * get name as a string
     */
    inline std::string name_str() const {
        return name_view().str();
    }

    /**
     * get the member container child or otherwise extendable member container child
     */
    ASTNode *child(const chem::string_view &child_name) {
        auto found = MembersContainer::child(child_name);
        if(found) return found;
        return ExtendableBase::extended_child(child_name);
    }

    /**
     * returns itself as extendable members container
     */
    ExtendableBase *as_extendable_members_container() final {
        return this;
    }

    /**
     * returns itself as extendable members container
     */
    ExtendableMembersContainerNode *as_extendable_members_container_node() final {
        return this;
    }

    inline void shallow_copy_into(ExtendableMembersContainerNode& other, ASTAllocator& allocator) {
        MembersContainer::shallow_copy_into(other, allocator);
        other.extension_functions = extension_functions;
    }

#ifdef COMPILER_BUILD

    /**
     * this method is dedicated for this extendable containers to externally declare themselves
     * this should be called in code_gen_external_declare, which is called upon nodes to declare
     * themselves in other modules when they are imported
     * this declares the functions inside the container (like MembersContainer) but it also
     * externally declares the extension functions inside this container
     */
    void extendable_external_declare(Codegen& gen) {
        external_declare(gen);
        for(auto& pair : extension_functions) {
            pair.second->code_gen_external_declare(gen);
        }
    }

#endif

};
