// Copyright (c) Qinetik 2024.

#include "Break.h"
#include "ast/base/LoopASTNode.h"

void BreakStatement::interpret(InterpretScope &scope) {
    if(node == nullptr) {
    scope.error("[Break] statement has nullptr to loop node");
    return;
    }
    node->body.stopInterpretOnce();
    node->stopInterpretation();
}