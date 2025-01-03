// Copyright (c) Qinetik 2024.

#include "ValueNode.h"

void ValueNode::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    value->link(linker, value);
}