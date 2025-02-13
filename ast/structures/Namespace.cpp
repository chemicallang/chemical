// Copyright (c) Qinetik 2024.

#include "Namespace.h"
#include "compiler/SymbolResolver.h"

void Namespace::declare_node(SymbolResolver& linker, ASTNode* node, const chem::string_view& node_id) {
    auto found = extended.find(node_id);
    if(found == extended.end()) {
        extended[node_id] = node;
    } else {
        linker.dup_sym_error(chem::string_view(node_id.data(), node_id.size()), found->second, node);
    }
}

void Namespace::declare_extended_in_linker(SymbolResolver& linker) {
    for(auto& node_pair : extended) {
        linker.declare(chem::string_view(node_pair.first.data(), node_pair.first.size()), node_pair.second);
    }
}

void Namespace::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto previous = linker.find(name());
    if(previous) {
        root = previous->as_namespace();
        if(root) {
            if(specifier() < root->specifier()) {
                linker.error("access specifier of this namespace must be at least '" + to_string(root->specifier()) + "' to match previous", this);
                return;
            }
            linker.scope_start();
            root->declare_extended_in_linker(linker);
            for(auto& node : nodes) {
                node->declare_top_level(linker, node);
                root->declare_node(linker, node, node->get_located_id()->identifier);
            }
            linker.scope_end();
        } else {
            linker.dup_sym_error(name(), previous, this);
        }
    } else {
        linker.declare_node(name(), this, specifier(), false);
        // we do not check for duplicate symbols here, because nodes are being declared first time
        for(const auto node : nodes) {
            extended[node->get_located_id()->identifier] = node;
        }
    }
}

void Namespace::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    if(root) {
        root->declare_extended_in_linker(linker);
    } else {
        for(auto& node : nodes) {
            node->declare_top_level(linker, node);
        }
    }
    for(const auto node : nodes) {
        node->link_signature(linker);
    }
    for(auto& node : nodes) {
        node->declare_and_link(linker, node);
    }
    linker.scope_end();
}

ASTNode *Namespace::child(const chem::string_view &child_name) {
    auto node = extended.find(child_name);
    if(node != extended.end()) {
        return node->second;
    }
    return nullptr;
}