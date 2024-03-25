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

void GlobalInterpretScope::add_error(const std::string &err) {
#ifdef DEBUG
    std::cerr << ANSI_COLOR_RED << "[InterpretError] " << err << ANSI_COLOR_RESET << std::endl;
#endif
    errors.emplace_back(err);
}

void GlobalInterpretScope::clean() {
    InterpretScope::clean();
    errors.clear();
    curr_node_position = 0;
}

GlobalInterpretScope::~GlobalInterpretScope() {
#ifdef DEBUG
    if (nodes_interpreted == -1 && warn_no_nodes) {
        std::cerr << ANSI_COLOR_RED
                  << "global nodes_interpreted = -1 , either the scope is empty, or scope doesn't increment nodes_interpreted"
                  << std::endl;
    }
#endif
    while (nodes_interpreted > -1) {
        codeScope->nodes[nodes_interpreted]->interpret_scope_ends(*this);
        nodes_interpreted--;
    }
}