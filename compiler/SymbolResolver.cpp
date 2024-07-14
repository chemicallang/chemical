// Copyright (c) Qinetik 2024.


#include "ast/base/ASTNode.h"
#include "SymbolResolver.h"
#include "ast/structures/MultiFunctionNode.h"
#include "ast/structures/FunctionDeclaration.h"

SymbolResolver::SymbolResolver(bool is64Bit) : ASTDiagnoser(), is64Bit(is64Bit) {

}

void SymbolResolver::dup_sym_error(const std::string& name, ASTNode* previous, ASTNode* new_node) {
    std::string err("duplicate symbol being declared " + name + " symbol already exists\n");
    err.append("previous : " + previous->representation() + "\n");
    err.append("new : " + new_node->representation() + "\n");
    error(err);
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
        dup_sym_error(name, found->second, node);
    }
}

void SymbolResolver::declare_function(const std::string& name, FunctionDeclaration* declaration) {
    if(name == "_") return;
    auto& last = current.back();
    auto found = last.find(name);
    if(found == last.end()) {
        last[name] = declaration;
    } else {
        auto result = handle_name_overridable_function(name, found->second, declaration);
        if(!result.duplicates.empty()) {
            for(auto dup : result.duplicates) {
                dup_sym_error(name, dup, declaration);
            }
        } else if(result.new_multi_func_node) {
            helper_nodes.emplace_back(result.new_multi_func_node);
            // override the previous symbol
            last[name] = result.new_multi_func_node;
        }
    }
}