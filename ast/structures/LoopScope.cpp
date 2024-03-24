// Copyright (c) Qinetik 2024.

#include "LoopScope.h"
#include "ast/base/GlobalInterpretScope.h"

void LoopScope::interpret(InterpretScope &scope) {
    scope.nodes_interpreted = -1;
    for(const auto& node : nodes) {
        node->position = scope.global->curr_node_position;
        node->interpret(scope);
        scope.global->curr_node_position++;
        scope.nodes_interpreted++;
        if(stoppedInterpretOnce) {
            stoppedInterpretOnce = false;
            return;
        }
    }
}