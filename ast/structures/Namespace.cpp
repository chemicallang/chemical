// Copyright (c) Qinetik 2024.

#include "Namespace.h"
#include "compiler/SymbolResolver.h"

Namespace::Namespace(std::string name, ASTNode* parent_node) : name(std::move(name)), parent_node(parent_node) {

}

void Namespace::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    auto previous = linker.find(name);
    if(previous) {
        root = previous->as_namespace();
        if(root) {

        } else {
            linker.error("a node exists by same name, the namespace with name '" + name + "' couldn't be created", this);
        }
    } else {
        linker.declare(name, this);
        for(auto& node : nodes) {
            extended[node->ns_node_identifier()] = node.get();
        }
    }
}

void Namespace::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    linker.scope_start();
    if(root) {
        for(auto& node : root->extended) {
            node.second->redeclare_top_level(linker, (std::unique_ptr<ASTNode>&) node.second);
        }
        for(auto& node : nodes) {
            node->declare_top_level(linker, node);
            root->extended[node->ns_node_identifier()] = node.get();
        }
    } else {
        for(auto& node : nodes) {
            node->declare_top_level(linker, node);
        }
    }
    for(auto& node : nodes) {
        node->declare_and_link(linker, node);
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