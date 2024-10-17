// Copyright (c) Qinetik 2024.

#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/base/Value.h"
#include "UsingStmt.h"

UsingStmt::UsingStmt(
    std::vector<ChainValue*> values,
    ASTNode* parent_node,
    bool is_namespace,
    CSTToken* token
) : chain(std::move(values), parent_node, false, nullptr), is_namespace(is_namespace), token(token) {

}

UsingStmt::UsingStmt(
    AccessChain* chain,
    bool is_namespace,
    CSTToken* token
) : chain(chain->values, chain->parent_node, chain->is_node, chain->token), is_namespace(is_namespace), token(token) {

}

void UsingStmt::declare_top_level(SymbolResolver &linker) {
    chain.declare_and_link(linker);
    auto linked = chain.linked_node();
    if(!linked) {
        linker.error("couldn't find linked node", this);
        return;
    }
    const auto no_propagate = linker.is_current_file_scope() && !has_annotation(AnnotationKind::Propagate);
    if(is_namespace) {
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
                auto& id = node->ns_node_identifier();
                if(no_propagate) {
                    linker.declare_file_disposable(id, node);
                } else {
                    linker.declare(id, node);
                }
            }
        } else {
            linker.error("expected value to be a namespace, however it isn't", this);
        }
    } else {
        auto& id = linked->ns_node_identifier();
        if(no_propagate) {
            linker.declare_file_disposable(id, linked);
        } else {
            linker.declare(id, linked);
        }
    }
}