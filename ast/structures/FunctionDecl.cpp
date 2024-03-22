// Copyright (c) Qinetik 2024.

#include "FunctionDeclaration.h"
#include "ast/base/GlobalInterpretScope.h"

void FunctionDeclaration::interpret_scope_ends(InterpretScope &scope) {
    scope.global->erase_node(name);
}