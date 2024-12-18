// Copyright (c) Qinetik 2024.

#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/base/Value.h"
#include "UsingStmt.h"

UsingStmt::UsingStmt(
    std::vector<ChainValue*> values,
    ASTNode* parent_node,
    bool is_namespace,
    SourceLocation location
) : chain(std::move(values), parent_node, false, location), location(location),
    attrs(is_namespace, false)
{

}

UsingStmt::UsingStmt(
    AccessChain* chain,
    bool is_namespace,
    SourceLocation location
) : chain(chain->values, chain->parent_node, chain->is_node, chain->location), location(location),
    attrs(is_namespace, false)
{

}

void UsingStmt::declare_top_level(SymbolResolver &linker) {
    chain.declare_and_link(linker);
    auto linked = chain.linked_node();
    if(!linked) {
        linker.error("couldn't find linked node", this);
        return;
    }
    const auto no_propagate = linker.is_current_file_scope() && !is_propagate();
    if(is_namespace()) {
        auto ns = linked->as_namespace();
        if(ns) {
//            for(const auto node : ns->nodes) {
//                auto& id = node->ns_node_identifier();
//                if(no_propagate) {
//                    linker.declare_file_disposable(id, node);
//                } else {
//                    linker.declare(id, node);
//                }
//            }
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
        node_id = linked->ns_node_identifier();
        if(no_propagate) {
            linker.declare_file_disposable(node_id, linked);
        } else {
            linker.declare(node_id, linked);
        }
    }
}