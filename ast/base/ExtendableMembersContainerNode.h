// Copyright (c) Qinetik 2024.

#pragma once

#include "ExtendableBase.h"
#include "ast/structures/MembersContainer.h"

class ExtendableMembersContainerNode : public MembersContainer, public ExtendableBase {
public:

    /**
     * the name of the node
     */
    std::string name;

    /**
     * constructor
     */
    explicit ExtendableMembersContainerNode(std::string name) : name(std::move(name)) {

    }

    /**
     * get the member container child or otherwise extendable member container child
     */
    ASTNode *child(const std::string &child_name) override {
        auto found = MembersContainer::child(child_name);
        if(found) return found;
        return ExtendableBase::extended_child(child_name);
    }

    /**
     * returns itself as extendable members container
     */
    ExtendableBase *as_extendable_members_container() override {
        return this;
    }

};
