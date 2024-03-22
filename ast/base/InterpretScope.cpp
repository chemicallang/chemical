// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "InterpretScope.h"
#include "GlobalInterpretScope.h"
#include "Value.h"
#include "ast/structures/Scope.h"

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

InterpretScope::InterpretScope(InterpretScope *parent, GlobalInterpretScope *global, Scope *scope, ASTNode *node)
        : parent(parent), global(global), codeScope(scope), node(node) {}

void InterpretScope::declare(const std::string &name, Value *value) {
    values[name] = value;
}

void InterpretScope::declare(const std::string &name, ASTNode *dec_node) {
    nodes[name] = dec_node;
}

ASTNode *InterpretScope::find_node(const std::string &name) {
    auto found = nodes.find(name);
    if (found == nodes.end()) {
        if(parent == nullptr) return nullptr;
        return parent->find_node(name);
    } else {
        return found->second;
    }
}

Value *InterpretScope::find_value(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return nullptr;
        return parent->find_value(name);
    } else {
        return found->second;
    }
}

std::pair<node_iterator, node_map&> InterpretScope::find_node_iterator(const std::string &name) {
    auto found = nodes.find(name);
    if (found == nodes.end()) {
        if(parent == nullptr) return {nodes.end(), nodes};
        return parent->find_node_iterator(name);
    } else {
        return {found, nodes};
    }
}

std::pair<value_iterator, value_map&> InterpretScope::find_value_iterator(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return {values.end(), values};
        return parent->find_value_iterator(name);
    } else {
        return {found, values};
    }
}

void InterpretScope::erase_value(const std::string &name) {
    auto iterator = find_value_iterator(name);
    if(iterator.first == iterator.second.end()) {
        std::cerr << ANSI_COLOR_RED << "couldn't locate value " << name << " for removal" << ANSI_COLOR_RESET
                  << std::endl;
#ifdef DEBUG
        print_values();
#endif
    } else {
        iterator.second.erase(iterator.first);
    }
}

void InterpretScope::erase_node(const std::string &name) {
    auto iterator = find_node_iterator(name);
    if(iterator.first == iterator.second.end()) {
        std::cerr << ANSI_COLOR_RED << "couldn't locate node " << name << " for removal" << ANSI_COLOR_RESET
                  << std::endl;
#ifdef DEBUG
        print_nodes();
#endif
    } else {
        iterator.second.erase(iterator.first);
    }
}

void InterpretScope::error(const std::string &err) {
    global->add_error(err);
}

void InterpretScope::print_values() {
    std::cout << "Values:" << std::endl;
    for (auto const &value: values) {
        std::cout << value.first << " : " << value.second->representation() << std::endl;
    }
}

void InterpretScope::print_nodes() {
    std::cout << "Nodes:" << std::endl;
    for (auto const &value: nodes) {
        std::cout << value.first << " : " << value.second->representation() << std::endl;
    }
}

InterpretScope::~InterpretScope() {
#ifdef DEBUG
    if (nodes_interpreted == -1) {

        // global interpret scope has its own destructor which also checks this
        // after global interpret scope's destructor is called, this destructor is called
        // because of inheritance, but the work is already done
        if (this == global) return;
        std::cerr << ANSI_COLOR_RED;
        std::cerr
                << "nodes_interpreted = -1 , either the scope is empty, or scope doesn't increment nodes_interpreted"
                << std::endl;
        if (node != nullptr) {
            std::cerr << "here's the representation of node : " << node->representation()
                      << std::endl;
        } else {

        }
        std::cerr << ANSI_COLOR_RESET;
    }
#endif
    while (nodes_interpreted > -1) {
        codeScope->nodes[nodes_interpreted]->interpret_scope_ends(*this);
        nodes_interpreted--;
    }
}