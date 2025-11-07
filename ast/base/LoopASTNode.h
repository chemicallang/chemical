// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 09/03/2024.
//

#pragma once

#include "ASTNode.h"
#include "ast/structures/Scope.h"

struct LoopASTNodeAttrs {
    /**
     * set by the break statement
     */
    bool has_break = false;
};

/**
 * Anything that's a loop (for, while..) inherits this LoopASTNode
 */
class LoopASTNode : public ASTNode {
public:

    Scope body;
    LoopASTNodeAttrs attrs;

    /**
     * constructor
     */
    LoopASTNode(
        Scope body,
        ASTNodeKind k,
        ASTNode* parent,
        SourceLocation loc
    ) : ASTNode(k, parent, loc), body(std::move(body)) {

    }

    /**
     * constructor
     */
    constexpr LoopASTNode(
        ASTNodeKind k,
        ASTNode* parent,
        SourceLocation loc
    ) : ASTNode(k, parent, loc), body(this, loc) {

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