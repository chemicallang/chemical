// Copyright (c) Qinetik 2024.

#include "LoopScope.h"

void LoopScope::interpret(InterpretScope &scope) {
    for (const auto &node: nodes) {
        node->interpret(scope);
        if (stoppedInterpretOnce) {
            stoppedInterpretOnce = false;
            return;
        }
    }
}


LoopScope::LoopScope(std::vector<ASTNode*> nodes, ASTNode* parent_node, SourceLocation location) : Scope(std::move(nodes), parent_node, location) {

}

void LoopScope::stopInterpretOnce() {
    stoppedInterpretOnce = true;
}