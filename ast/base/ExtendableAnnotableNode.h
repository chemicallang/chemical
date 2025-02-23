// Copyright (c) Qinetik 2024.

#pragma once

#include "ExtendableBase.h"
#include "ast/base/AnnotableNode.h"

class ExtendableNode : public ASTNode, public ExtendableBase {
public:

    /**
     * constructor
     */
    inline explicit ExtendableNode(ASTNodeKind k) noexcept : ASTNode(k) {

    }

    /**
     * get the member container child or otherwise extendable member container child
     */
    ASTNode *child(const chem::string_view &name) {
        auto found = ASTNode::child(name);
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

using ExtendableAnnotableNode = ExtendableNode;