// Copyright (c) Qinetik 2024.

#include "UnionType.h"
#include "ast/structures/VariablesContainer.h"
#include "compiler/SymbolResolver.h"

bool UnionType::link(SymbolResolver &linker) {
    for(auto& var : variables) {
        if(!var.second->known_type()->link(linker)) {
            return false;
        }
    }
    if(!name.empty()) {
        linker.declare(name, this);
    }
    return true;
}