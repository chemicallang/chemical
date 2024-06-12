// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"

SymbolResolver::SymbolResolver(const std::string& path, bool is64Bit) : ASTDiagnoser(path), is64Bit(is64Bit) {

}

void SymbolResolver::declare(const std::string& name, ASTNode* node) {
    if(name == "_") {
        // symbols with name '_' aren't declared
        return;
    }
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