// Copyright (c) Qinetik 2024.

#include "PointerType.h"

void PointerType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    type->link(linker, type);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}