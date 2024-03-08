// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"

#include <utility>

GlobalInterpretScope::GlobalInterpretScope(InterpretScope* parent, Scope* scope, ASTNode* node, std::string path) : root_path(std::move(path)), InterpretScope(parent, this, scope, node) {

}

void GlobalInterpretScope::add_error(const std::string &err) {
    errors.emplace_back(err);
}