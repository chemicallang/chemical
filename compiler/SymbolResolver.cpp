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
        auto previous = found->second->as_function();
        auto multi = found->second->as_multi_func_node();
        if(multi) {
            bool failed = false;
            for(auto func : multi->functions) {
                if(func->do_param_types_match(declaration->params)) {
                    dup_sym_error(name, func, declaration);
                    failed = true;
                }
            }
            if(failed) return;
            multi->functions.emplace_back(declaration);
        } else if(previous) {
            if(!previous->do_param_types_match(declaration->params)) {
                multi = new MultiFunctionNode(name);
                multi->functions.emplace_back(previous);
                multi->functions.emplace_back(declaration);
                helper_nodes.emplace_back(multi);
                // override the previous symbol
                last[name] = multi;
            } else {
                dup_sym_error(name, found->second, declaration);
            }
        } else {
            dup_sym_error(name, found->second, declaration);
        }
    }
}