// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ExtendableBase.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/GenericFuncDecl.h"

class ExtendableMembersContainerNode : public MembersContainer {
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
     * returns itself as extendable members container
     */
    ExtendableMembersContainerNode *as_extendable_members_container_node() final {
        return this;
    }

    inline void shallow_copy_into(ExtendableMembersContainerNode& other, ASTAllocator& allocator) {
        MembersContainer::shallow_copy_into(other, allocator);
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
        for(const auto decl : extension_functions) {
            decl->code_gen_external_declare(gen);
        }
    }

#endif

};
