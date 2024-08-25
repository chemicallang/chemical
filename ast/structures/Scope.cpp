// Copyright (c) Qinetik 2024.

#include "Scope.h"
#include <iostream>
#include "ast/base/GlobalInterpretScope.h"

void Scope::interpret(InterpretScope &scope) {
    for (const auto &node: nodes) {
        node->interpret(scope);
    }
}

Scope::Scope(std::vector<std::unique_ptr<ASTNode>> nodes, ASTNode* parent_node) : nodes(std::move(nodes)), parent_node(parent_node) {

}

void Scope::accept(Visitor *visitor) {
    visitor->visit(this);
}

#ifdef DEBUG
void Scope::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    throw std::runtime_error("Scope::declare_top_level shouldn't be called, other link methods should be called");
}
void Scope::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    throw std::runtime_error("Scope::declare_and_link shouldn't be called, other link methods should be called");
}
#endif

void Scope::link_sequentially(SymbolResolver &linker) {
    for (auto &node: nodes) {
        node->declare_top_level(linker, node);
        node->declare_and_link(linker, node);
    }
}

void Scope::link_asynchronously(SymbolResolver &linker) {
    for (auto &node: nodes) {
        node->declare_top_level(linker, node);
    }
    for (auto &node: nodes) {
        node->declare_and_link(linker, node);
    }
}

void Scope::stopInterpretOnce() {

}