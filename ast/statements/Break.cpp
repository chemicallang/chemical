// Copyright (c) Chemical Language Foundation 2025.

#include "Break.h"
#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"

void BreakStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(value) {
        value->link(linker, value);
    }
}

void stop_interpretation_above_once(ASTNode* node) {
    if(ASTNode::isLoopASTNode(node->kind())) {
        const auto loop_node = node->as_loop_node_unsafe();
        loop_node->body.stopInterpretOnce();
        loop_node->stopInterpretation();
        return;
    }
    const auto parent = node->parent();
    if(parent) {
        stop_interpretation_above_once(parent);
    }
}

void BreakStatement::interpret(InterpretScope &scope) {
    stop_interpretation_above_once(parent());
}