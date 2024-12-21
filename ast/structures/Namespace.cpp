// Copyright (c) Qinetik 2024.

#include "Namespace.h"
#include "compiler/SymbolResolver.h"

Namespace::Namespace(
    LocatedIdentifier identifier,
    ASTNode* parent_node,
    SourceLocation location,
    AccessSpecifier specifier
) : identifier(identifier), parent_node(parent_node), location(location), attrs(specifier, false) {

}

void Namespace::declare_node(SymbolResolver& linker, ASTNode* node, const std::string& node_id) {
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

void Namespace::declare_top_level(SymbolResolver &linker) {
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
            for(const auto node : nodes) {
                node->declare_top_level(linker);
                root->declare_node(linker, node, node->ns_node_identifier());
            }
            linker.scope_end();
        } else {
            linker.dup_sym_error(name(), previous, this);
        }
    } else {
        linker.declare_node(name(), this, specifier(), false);
        // we do not check for duplicate symbols here, because nodes are being declared first time
        for(const auto node : nodes) {
            extended[node->ns_node_identifier()] = node;
        }
    }
}

void Namespace::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    if(root) {
        root->declare_extended_in_linker(linker);
    } else {
        for(const auto node : nodes) {
            node->declare_top_level(linker);
        }
    }
    for(const auto node : nodes) {
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