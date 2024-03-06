// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#include "GlobalInterpretScope.h"

GlobalInterpretScope::GlobalInterpretScope(Scope* scope, ASTNode* node) : InterpretScope(nullptr, this, scope, node) {

}

void GlobalInterpretScope::add_error(const std::string &err) {
    errors.emplace_back(err);
}