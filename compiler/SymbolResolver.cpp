// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"

SymbolResolver::SymbolResolver(std::string curr_exe_path, const std::string& path, bool is64Bit) : ASTProcessor(std::move(curr_exe_path), path), is64Bit(is64Bit) {

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
    if(found == last.end() || override_symbols) {
        last[name] = node;
    } else {
        std::string err("duplicate symbol being declared " + name + " symbol already exists\n");
        err.append("previous : " + found->second->representation() + "\n");
        err.append("new : " + node->representation() + "\n");
        error(err);
    }
}