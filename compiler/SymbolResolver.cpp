// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"

SymbolResolver::SymbolResolver(std::string path) : path(std::move(path)) {

}

ASTNode *SymbolResolver::find(const std::string &name) {
    auto found = current.find(name);
    if(found == current.end()) {
        return nullptr;
    } else {
        return found->second;
    }
}

void SymbolResolver::declare(const std::string& name, ASTNode* node) {
    auto found = current.find(name);
    if(found == current.end()) {
        current[name] = node;
    } else {
        error("duplicate symbol being declared " + name + " symbol already exists with node representation : " + found->second->representation());
    }
}

void SymbolResolver::erase(const std::string &name) {
    auto found = current.find(name);
    if(found == current.end()) {
        error("couldn't find symbol to erase " + name);
    } else {
        current.erase(found);
    }
}