// Copyright (c) Qinetik 2024.

#include "Namespace.h"
#include "compiler/SymbolResolver.h"

Namespace::Namespace(std::string name) : name(std::move(name)) {

}

void Namespace::declare_top_level(SymbolResolver &linker) {
    auto previous = linker.find(name);
    if(previous) {
        auto root = previous->as_namespace();
        if(root) {
            for(auto& node : nodes) {
                root->extended[node->ns_node_identifier()] = node.get();
            }
        } else {
            linker.error("a node exists by same name, the namespace with name '" + name + "' couldn't be created");
        }
    } else {
        linker.declare(name, this);
        for(auto& node : nodes) {
            extended[node->ns_node_identifier()] = node.get();
        }
    }
}

void Namespace::declare_and_link(SymbolResolver &linker) {
    for(auto& node : nodes) {
        node->declare_top_level(linker);
    }
    for(auto& node : nodes) {
        node->declare_and_link(linker);
    }
}

ASTNode *Namespace::child(const std::string &child_name) {
    auto node = extended.find(child_name);
    if(node != extended.end()) {
        return node->second;
    }
    return nullptr;
}