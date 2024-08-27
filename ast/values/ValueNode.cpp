// Copyright (c) Qinetik 2024.

#include "ValueNode.h"

void ValueNode::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) {
    value->link(linker, value);
}