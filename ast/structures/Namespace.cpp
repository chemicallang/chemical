// Copyright (c) Chemical Language Foundation 2025.

#include "Namespace.h"
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
        linker.declare_or_shadow(node_pair.first, node_pair.second);
    }
}

void Namespace::put_in_extended(std::unordered_map<chem::string_view, ASTNode*>& extended) {
    MapSymbolDeclarer declarer(extended);
    for(const auto node : nodes) {
        ::declare_node(declarer, node, AccessSpecifier::Private);
    }
}