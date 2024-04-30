// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"

SymbolResolver::SymbolResolver(std::string path) : path(std::move(path)) {

}

void SymbolResolver::scope_start() {
    current.emplace_back();
}

void SymbolResolver::scope_end() {
    current.pop_back();
}

ASTNode *SymbolResolver::find(const std::string &name) {
    int i = current.size() - 1;
    std::unordered_map<std::string, ASTNode*>* last;
    while(i >= 0) {
        last = &current[i];
        auto found = last->find(name);
        if(found != last->end()) {
            return found->second;
        }
        i--;
    }
    return nullptr;
}

void SymbolResolver::declare(const std::string& name, ASTNode* node) {
    auto& last = current.back();
    auto found = last.find(name);
    if(found == last.end()) {
        last[name] = node;
    } else {
        error("duplicate symbol being declared " + name + " symbol already exists with node representation : " + found->second->representation());
    }
}