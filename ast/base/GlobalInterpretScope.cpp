// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"
#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"

#include <utility>
#include <iostream>

#define ANSI_COLOR_RED     "\x1b[91m"
#define ANSI_COLOR_RESET   "\x1b[0m"

GlobalInterpretScope::GlobalInterpretScope(InterpretScope* parent, Scope* scope, ASTNode* node, std::string path) : root_path(std::move(path)), InterpretScope(parent, this, scope, node) {

}

void GlobalInterpretScope::print_values() {
    std::cout << "Values:" << std::endl;
    for (auto const &value: values) {
        std::cout << value.first << " : " << value.second->representation() << std::endl;
    }
}

void GlobalInterpretScope::print_nodes() {
    std::cout << "Nodes:" << std::endl;
    for (auto const &value: nodes) {
        std::cout << value.first << " : " << value.second->representation() << std::endl;
    }
}

void GlobalInterpretScope::erase_value(const std::string& name) {
    auto found = global->values.find(name);
    if(found == global->values.end()) {
        std::cerr << ANSI_COLOR_RED << "couldn't locate value " << name << " for removal" << ANSI_COLOR_RESET << std::endl;
#ifdef DEBUG
      print_values();
#endif
    } else {
        global->values.erase(found);
    }
}

void GlobalInterpretScope::erase_node(const std::string& name) {
    auto found = global->nodes.find(name);
    if(found == global->nodes.end()) {
        std::cerr << ANSI_COLOR_RED << "couldn't locate node " << name << " for removal" << ANSI_COLOR_RESET << std::endl;
#ifdef DEBUG
    print_nodes();
#endif
    } else {
        global->nodes.erase(found);
    }
}

void GlobalInterpretScope::add_error(const std::string &err) {
    errors.emplace_back(err);
}