// Copyright (c) Qinetik 2024.

#include "Scope.h"
#include "ast/base/GlobalInterpretScope.h"

void Scope::interpret(InterpretScope &scope) {
    scope.nodes_interpreted = -1;
    for (const auto &node: nodes) {
        node->position = scope.global->curr_node_position;
        node->interpret(scope);
        scope.global->curr_node_position++;
        scope.nodes_interpreted++;
    }
}

Scope::Scope(std::vector<std::unique_ptr<ASTNode>> nodes) : nodes(std::move(nodes)) {

}

Scope::Scope(Scope &&other) noexcept : nodes(std::move(other.nodes)) {

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