// Copyright (c) Qinetik 2024.

#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/base/Value.h"
#include "UsingStmt.h"

UsingStmt::UsingStmt(
    AccessChain* chain,
    ASTNode* parent_node,
    bool is_namespace,
    SourceLocation location
) : chain(chain), parent_node(parent_node), location(location),
    attrs(is_namespace, false)
{

}

void UsingStmt::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    chain->link(linker, nullptr, nullptr, 0, true, false);
    auto linked = chain->linked_node();
    if(!linked) {
        linker.error("couldn't find linked node", this);
        return;
    }
    const auto no_propagate = linker.is_current_file_scope() && !is_propagate();
    if(is_namespace()) {
        auto ns = linked->as_namespace();
        if(ns) {
            for(auto& node_pair : ns->extended) {
                const auto node = node_pair.second;
                if(no_propagate) {
                    linker.declare_file_disposable(chem::string_view(node_pair.first.data(), node_pair.first.size()), node);
                } else {
                    linker.declare(chem::string_view(node_pair.first.data(), node_pair.first.size()), node);
                }
            }
        } else {
            linker.error("expected value to be a namespace, however it isn't", this);
        }
    } else {
        const auto& name_view = linked->get_located_id()->identifier;
        if(no_propagate) {
            linker.declare_file_disposable(name_view, linked);
        } else {
            linker.declare(name_view, linked);
        }
    }
}