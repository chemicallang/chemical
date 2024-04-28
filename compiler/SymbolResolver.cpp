// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"

SymbolResolver::SymbolResolver(std::string path) : path(std::move(path)) {

}

void SymbolResolver::declare(const std::string& name, ASTNode* node) {
    auto found = current.find(name);
    if(found == current.end()) {
        current[name] = node;
    } else {
        error("duplicate symbol being declared " + name + " symbol already exists with node representation : " + found->second->representation());
    }
}