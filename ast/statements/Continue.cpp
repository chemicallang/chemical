// Copyright (c) Qinetik 2024.

#include "Continue.h"

/**
 * @brief Construct a new ContinueStatement object.
 */
ContinueStatement::ContinueStatement(LoopASTNode *node) : node(node) {}

void ContinueStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

void ContinueStatement::interpret(InterpretScope &scope) {
    if (node == nullptr) {
        scope.error("[Continue] statement has nullptr to loop node");
        return;
    }
    node->body.stopInterpretOnce();
}
