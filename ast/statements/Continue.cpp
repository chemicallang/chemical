// Copyright (c) Chemical Language Foundation 2025.

#include "Continue.h"
#include "ast/base/InterpretScope.h"

void skip_interpretation_above_once(ASTNode* node) {
    if(ASTNode::isLoopASTNode(node->kind())) {
        node->as_loop_node_unsafe()->body.stopInterpretOnce();
        return;
    }
    const auto parent = node->parent();
    if(parent) {
        skip_interpretation_above_once(parent);
    }
}


void ContinueStatement::interpret(InterpretScope &scope) {
    skip_interpretation_above_once(parent());
}
