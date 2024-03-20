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

void InterpretScope::error(const std::string &err) {
#ifdef DEBUG
    std::cerr << ANSI_COLOR_RED << "[InterpretError]" << err << ANSI_COLOR_RESET << std::endl;
#endif
    global->add_error(err);
}

InterpretScope::~InterpretScope() {
#ifdef DEBUG
    if (nodes_interpreted == -1) {
        std::cerr << ANSI_COLOR_RED
                  << "nodes_interpreted = -1 , shouldn't be, either the scope is empty, or scope doesn't increment nodes_interpreted when interpreting nodes"
                  << std::endl;
        if (node == nullptr) return;
        std::cerr << "here's the representation of node : " << node->representation() << ANSI_COLOR_RESET << std::endl;
    }
#endif
    while (nodes_interpreted > -1) {
        codeScope->nodes[nodes_interpreted]->interpret_scope_ends(*this);
        nodes_interpreted--;
    }
}