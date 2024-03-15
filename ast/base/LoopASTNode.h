// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#pragma once

#include "ASTNode.h"
#include "ast/structures/LoopScope.h"

/**
 * Anything that's a loop (for, while..) inherits this LoopASTNode
 */
class LoopASTNode : public ASTNode {
public:

    LoopScope body;

    /**
     * Initialize an empty loop ast node
     */
    LoopASTNode() {

    }

    /**
     * initialize with the given body
     * @param body
     */
    LoopASTNode(LoopScope body) : body(std::move(body)) {

    }

    /**
     * This is called by statements like break
     * to break the current interpretation, that is run by ASTNode's like loops (for, while)
     * While this does break the loop, BUT the loop will break on the next iteration
     * To break the current iteration at current statement inside the loop, this will be called along with stopInterpretation on the body scope
     */
    virtual void stopInterpretation() {

    }

};