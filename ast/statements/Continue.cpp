// Copyright (c) Chemical Language Foundation 2025.

#include "Continue.h"
#include "ast/base/InterpretScope.h"

void ContinueStatement::interpret(InterpretScope &scope) {
//    if (node == nullptr) {
//        scope.error("[Continue] statement has nullptr to loop node", this);
//        return;
//    }
    node->body.stopInterpretOnce();
}
