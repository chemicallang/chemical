// Copyright (c) Qinetik 2024.

#include "MultiFunctionNode.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/FunctionDeclaration.h"

MultiFunctionNode::MultiFunctionNode(std::string name) : name(std::move(name)) {

}

void MultiFunctionNode::declare_and_link(SymbolResolver &linker) {

    // link all the functions
    for(auto func : functions) {
        func->declare_and_link(linker);
    }

}
