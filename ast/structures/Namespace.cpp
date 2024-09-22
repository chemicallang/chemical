// Copyright (c) Qinetik 2024.

#include "Namespace.h"
#include "compiler/SymbolResolver.h"

Namespace::Namespace(
    std::string name,
    ASTNode* parent_node,
    CSTToken* token,
    AccessSpecifier specifier
) : name(std::move(name)), parent_node(parent_node), token(token), specifier(specifier) {

}

void Namespace::declare_top_level(SymbolResolver &linker) {
    auto previous = linker.find(name);
    if(previous) {
        root = previous->as_namespace();
        if(root) {

        } else {
            linker.error("a node exists by same name, the namespace with name '" + name + "' couldn't be created", this);
        }
    } else {
        linker.declare_node(name, this, specifier, false);
        for(auto& node : nodes) {
            extended[node->ns_node_identifier()] = node;
        }
    }
}

void Namespace::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    if(root) {
        for(auto& node : root->extended) {
            node.second->redeclare_top_level(linker);
        }
        for(auto& node : nodes) {
            node->declare_top_level(linker);
            root->extended[node->ns_node_identifier()] = node;
        }
    } else {
        for(auto& node : nodes) {
            node->declare_top_level(linker);
        }
    }
    for(auto& node : nodes) {
        node->declare_and_link(linker);
    }
    linker.scope_end();
}

ASTNode *Namespace::child(const std::string &child_name) {
    auto node = extended.find(child_name);
    if(node != extended.end()) {
        return node->second;
    }
    return nullptr;
}