// Copyright (c) Qinetik 2024.

#include "Scope.h"
#include "ast/base/GlobalInterpretScope.h"

void Scope::interpret(InterpretScope &scope) {
    for (const auto &node: nodes) {
        node->position = scope.global->curr_node_position;
        node->interpret(scope);
        scope.global->curr_node_position++;
    }
}

Scope::Scope(std::vector<std::unique_ptr<ASTNode>> nodes, ASTNode* parent_node) : nodes(std::move(nodes)), parent_node(parent_node) {

}

Scope::Scope(Scope &&other) noexcept : nodes(std::move(other.nodes)), parent_node(other.parent_node) {

}

void Scope::accept(Visitor *visitor) {
    visitor->visit(this);
}

void Scope::declare_top_level(SymbolResolver &linker) {
    for (const auto &node: nodes) {
        node->declare_top_level(linker);
    }
}

void Scope::declare_and_link(SymbolResolver &linker) {
    for (const auto &node: nodes) {
        node->declare_and_link(linker);
    }
}

void Scope::stopInterpretOnce() {

}