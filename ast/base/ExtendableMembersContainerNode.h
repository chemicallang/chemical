// Copyright (c) Qinetik 2024.

#pragma once

#include "ExtendableBase.h"
#include "ast/structures/MembersContainer.h"

class ExtendableMembersContainerNode : public MembersContainer, public ExtendableBase {
public:

    /**
     * get the member container child or otherwise extendable member container child
     */
    ASTNode *child(const std::string &name) override {
        auto found = MembersContainer::child(name);
        if(found) return found;
        return ExtendableBase::extended_child(name);
    }

    /**
     * returns itself as extendable members container
     */
    ExtendableBase *as_extendable_members_container() override {
        return this;
    }

};
