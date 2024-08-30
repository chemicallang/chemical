// Copyright (c) Qinetik 2024.

#include "Break.h"
#include "ast/base/LoopASTNode.h"
#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"

BreakStatement::BreakStatement(LoopASTNode *node, ASTNode* parent_node) : node(node), parent_node(parent_node), value(nullptr) {}

void BreakStatement::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) {
    if(value) {
        value->link(linker, value);
    }
}

void BreakStatement::interpret(InterpretScope &scope) {
    if(node == nullptr) {
    scope.error("[Break] statement has nullptr to loop node");
    return;
    }
    node->body.stopInterpretOnce();
    node->stopInterpretation();
}