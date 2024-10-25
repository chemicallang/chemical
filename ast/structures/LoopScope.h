// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "Scope.h"

class LoopScope : public Scope {
public:

    bool stoppedInterpretOnce = false;

    /**
     * empty constructor
     */
    LoopScope(ASTNode* parent_node, SourceLocation location) : Scope(parent_node, location) {

    }

    /**
     * construct with given nodes
     * @param nodes
     */
    LoopScope(std::vector<ASTNode*> nodes, ASTNode* parent_node, SourceLocation location);

    /**
     * this just continuously interprets nodes in scope, without stopping unless user
     * uses one of break or return statements
     */
    void interpret(InterpretScope& scope) final;

    /**
     * this will stop the interpretation at the current statement
     * meaning all the nodes in this scope will be skipped
     */
    void stopInterpretOnce() final;

};