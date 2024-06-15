// Copyright (c) Qinetik 2024.

#pragma once

#include "ASTNode.h"
#include "Annotation.h"

class AnnotableNode : public ASTNode {
public:

    /**
     * Annotations that this node contains
     */
    std::vector<Annotation> annotations;

};