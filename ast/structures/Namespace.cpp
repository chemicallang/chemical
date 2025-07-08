// Copyright (c) Chemical Language Foundation 2025.

#include "Namespace.h"
#include "compiler/SymbolResolver.h"
#include "compiler/symres/NodeSymbolDeclarer.h"
#include "compiler/symres/DeclareTopLevel.h"

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

void Namespace::put_in_extended(std::unordered_map<chem::string_view, ASTNode*>& extended) {
    MapSymbolDeclarer declarer(extended);
    for(const auto node : nodes) {
        ::declare_node(declarer, node, AccessSpecifier::Private);
    }
}

void Namespace::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    if(root) {
        root->declare_extended_in_linker(linker);
    } else {
        TopLevelDeclSymDeclare declarer(linker);
        for(auto& node : nodes) {
            declarer.visit(node);
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