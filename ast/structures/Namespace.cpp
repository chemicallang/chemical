// Copyright (c) Chemical Language Foundation 2025.

#include "Namespace.h"
#include "compiler/SymbolResolver.h"
#include "compiler/symres/NodeSymbolDeclarer.h"

void Namespace::declare_node(SymbolResolver& linker, ASTNode* node, const chem::string_view& node_id) {
    auto found = extended.find(node_id);
    if(found == extended.end()) {
        extended[node_id] = node;
    } else {
        linker.dup_sym_error(node_id, found->second, node);
    }
}

void Namespace::declare_extended_in_linker(SymbolResolver& linker) {
    for(auto& node_pair : extended) {
        linker.declare(node_pair.first, node_pair.second);
    }
}

void Namespace::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto previous = linker.find(name());
    if(previous) {
        root = previous->as_namespace();
        if(root) {
            // namespace attributes are propagated to all namespaces with same name ?
            // TODO propagate namespace attributes
            // attrs = root->attrs;
            if(specifier() < root->specifier()) {
                linker.error(this) << "access specifier of this namespace must be at least '" << to_string(root->specifier()) << "' to match previous";
                return;
            }
            linker.scope_start();
            root->declare_extended_in_linker(linker);
            for(auto& node : nodes) {
                node->declare_top_level(linker, node);
            }
            // TODO we must check for duplicate symbols being declared in root_extended
            MapSymbolDeclarer declarer(root->extended);
            for(const auto node : nodes) {
                ::declare_node(declarer, node, AccessSpecifier::Private);
            }
            linker.scope_end();
        } else {
            linker.dup_sym_error(name(), previous, this);
        }
    } else {
        linker.declare_node(name(), this, specifier(), false);
        linker.scope_start();
        // declare top level all nodes inside the namespace
        for(auto& node : nodes) {
            node->declare_top_level(linker, node);
        }
        // we do not check for duplicate symbols here, because nodes are being declared first time
        MapSymbolDeclarer declarer(extended);
        for(const auto node : nodes) {
            ::declare_node(declarer, node, AccessSpecifier::Private);
        }
        linker.scope_end();
    }
}

void Namespace::link_signature(SymbolResolver &linker) {
    linker.scope_start();
    if(root) {
        root->declare_extended_in_linker(linker);
    } else {
        declare_extended_in_linker(linker);
    }
    for(const auto node : nodes) {
        node->link_signature(linker);
    }
    linker.scope_end();
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