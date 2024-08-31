// Copyright (c) Qinetik 2024.

#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/base/Value.h"
#include "UsingStmt.h"

UsingStmt::UsingStmt(
    std::vector<std::unique_ptr<ChainValue>> values,
    ASTNode* parent_node,
    bool is_namespace,
    CSTToken* token
) : chain(std::move(values), parent_node, false, nullptr), is_namespace(is_namespace), token(token) {

}

void UsingStmt::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    chain.declare_and_link(linker, (std::unique_ptr<ASTNode>&) chain);
    auto linked = chain.linked_node();
    if(!linked) {
        linker.error("couldn't find linked node", this);
        return;
    }
    const auto no_propagate = linker.is_current_file_scope() && !has_annotation(AnnotationKind::Propagate);
    if(is_namespace) {
        auto ns = linked->as_namespace();
        if(ns) {
            for(auto& node : ns->nodes) {
                auto& id = node->ns_node_identifier();
                linker.declare(id, node.get());
                if(no_propagate) {
                    linker.dispose_file_symbols.emplace_back(id);
                }
            }
        } else {
            linker.error("expected value to be a namespace, however it isn't", this);
        }
    } else {
        auto& id = linked->ns_node_identifier();
        linker.declare(id, linked);
        if(no_propagate) {
            linker.dispose_file_symbols.emplace_back(id);
        }
    }
}