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

Value *InterpretScope::find_value(const std::string &name) {
    auto found = values.find(name);
    if (found == values.end()) {
        if(parent == nullptr) return nullptr;
        return parent->find_value(name);
    } else {
        return found->second;
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

void InterpretScope::error(const std::string &err) {
    global->add_error(err);
}

void InterpretScope::print_values() {
    std::cout << "Values:" << std::endl;
    for (auto const &value: values) {
        std::cout << value.first << " : " << value.second->representation() << std::endl;
    }
    if(parent != nullptr) {
        std::cout << "Parent ";
        parent->print_values();
    }
}

void InterpretScope::clean() {
    values.clear();
    nodes_interpreted = -1;
}

InterpretScope::~InterpretScope() {
    if(this == global || parent == nullptr) {
        // global interpret scope has its own destructor which also checks this
        // after global interpret scope's destructor is called, this destructor is called
        // because of inheritance, but the work is already done
        if(nodes_interpreted == -1) {
            return;
        } else {
#ifdef DEBUG
    std::cerr << ANSI_COLOR_RED;
    std::cerr << "global | non owned interpret scope has nodes interpreted != -1" << std::endl;
    std::cerr << ANSI_COLOR_RESET;
#endif
        }
    }
#ifdef DEBUG
    if (nodes_interpreted == -1 && (codeScope || node)) {

        std::cerr << ANSI_COLOR_RED;
        std::cerr
                << "nodes_interpreted = -1 , either the scope is empty, or scope doesn't increment nodes_interpreted"
                << std::endl;
        if (node != nullptr) {
            std::cerr << "here's the representation of node : " << node->representation()
                      << std::endl;
        } else {
            std::cerr << "nodes present in the current scope\n";
            for(const auto& node : codeScope->nodes) {
                std::cerr << node->representation() + '\n';
            }
            std::cerr << std::endl;
        }
        std::cerr << ANSI_COLOR_RESET;
    }
#endif
    while (nodes_interpreted > -1) {
        codeScope->nodes[nodes_interpreted]->interpret_scope_ends(*this);
        nodes_interpreted--;
    }
}