// Copyright (c) Qinetik 2024.

#include "LoopScope.h"
#include "ast/base/GlobalInterpretScope.h"

void LoopScope::interpret(InterpretScope &scope) {
    for (const auto &node: nodes) {
        node->position = scope.global->curr_node_position;
        node->interpret(scope);
        scope.global->curr_node_position++;
        if (stoppedInterpretOnce) {
            stoppedInterpretOnce = false;
            return;
        }
    }
}


LoopScope::LoopScope(std::vector<std::unique_ptr<ASTNode>> nodes, ASTNode* parent_node) : Scope(std::move(nodes), parent_node) {

}

void LoopScope::stopInterpretOnce() {
    stoppedInterpretOnce = true;
}