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
    global->add_error(err);
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