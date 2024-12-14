// Copyright (c) Qinetik 2024.

#pragma once

#include "ExtendableBase.h"
#include "ast/structures/MembersContainer.h"

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
    explicit ExtendableMembersContainerNode(LocatedIdentifier identifier) : identifier(std::move(identifier)) {

    }

    /**
     * get the name of the container
     */
    inline const std::string name() const {
        return identifier.identifier.str();
    }

    /**
     * get the name of the container
     */
    inline const chem::string_view& name_view() const {
        return identifier.identifier;
    }

    /**
     * runtime name for thi members container node
     */
    void runtime_name(std::ostream &stream) final;

    /**
     * get runtime name without parent
     */
    void runtime_name_no_parent(std::ostream &stream) final;

    /**
     * get runtime name without parent as string
     */
    std::string runtime_name();

    /**
     * get runtime name without parent as string
     */
    std::string runtime_name_no_parent_str();

    /**
     * get the member container child or otherwise extendable member container child
     */
    ASTNode *child(const std::string &child_name) {
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

};
