// Copyright (c) Chemical Language Foundation 2025.

#include "Break.h"
#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"

void BreakStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(value) {
        value->link(linker, value);
    }
}