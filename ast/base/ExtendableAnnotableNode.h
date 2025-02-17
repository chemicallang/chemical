// Copyright (c) Qinetik 2024.

#pragma once

#include "ExtendableBase.h"
#include "ast/base/AnnotableNode.h"

class ExtendableAnnotableNode : public AnnotableNode, public ExtendableBase {
public:

    /**
     * get the member container child or otherwise extendable member container child
     */
    ASTNode *child(const chem::string_view &name) {
        auto found = AnnotableNode::child(name);
        if(found) return found;
        return (ASTNode*) ExtendableBase::extended_child(name);
    }

    /**
     * returns itself as extendable members container
     */
    ExtendableBase *as_extendable_members_container() final {
        return this;
    }

};
