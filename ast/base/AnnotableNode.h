// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTNode.h"
#include "AnnotationParent.h"

class AnnotableNode : public ASTNode, public AnnotationParent {
public:

    AnnotableNode *as_annotable_node() override {
        return this;
    }

};