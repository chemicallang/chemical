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

Scope::Scope(std::vector<std::unique_ptr<ASTNode>> nodes) : nodes(std::move(nodes)) {}

Scope::Scope(Scope &&other) : nodes(std::move(other.nodes)) {}

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

#ifdef COMPILER_BUILD

void Scope::code_gen(Codegen &gen) {
    for(auto& node : nodes) {
        node->code_gen_declare(gen);
    }
    int i = 0;
    while(i < nodes.size()) {
//        std::cout << "Generating " + std::to_string(i) << std::endl;
        nodes[i]->code_gen(gen, nodes, i);
//        std::cout << "Success " + std::to_string(i) << " : " << nodes[i]->representation() << std::endl;
        i++;
    }
    i = nodes.size() - 1;
    while(i >= 0){
        nodes[i]->code_gen_destruct(gen, nodes, i);
        i--;
    }
}

#endif

void Scope::stopInterpretOnce() {

}

std::string Scope::representation() const {
    std::string rep;
    int i = 0;
    while (i < nodes.size()) {
        rep.append(nodes[i]->representation());
        if (i != nodes.size() - 1) {
            rep.append(1, '\n');
        }
        i++;
    }
    return rep;
}