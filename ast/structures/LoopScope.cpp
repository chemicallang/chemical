// Copyright (c) Qinetik 2024.

#include "LoopScope.h"
#include "ast/base/GlobalInterpretScope.h"

void LoopScope::interpret(InterpretScope &scope) {
    for (const auto &node: nodes) {
        node->interpret(scope);
        if (stoppedInterpretOnce) {
            stoppedInterpretOnce = false;
            return;
        }
    }
}


LoopScope::LoopScope(std::vector<ASTNode*> nodes, ASTNode* parent_node, CSTToken* token) : Scope(std::move(nodes), parent_node, token) {

}

void LoopScope::stopInterpretOnce() {
    stoppedInterpretOnce = true;
}