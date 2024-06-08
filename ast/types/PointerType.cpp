// Copyright (c) Qinetik 2024.

#include "PointerType.h"

void PointerType::link(SymbolResolver &linker) {
    type->link(linker);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}