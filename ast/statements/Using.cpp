// Copyright (c) Qinetik 2024.

#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/base/Value.h"
#include "UsingStmt.h"

UsingStmt::UsingStmt(
    std::vector<std::unique_ptr<Value>> values,
    ASTNode* parent_node,
    bool is_namespace
) : chain(std::move(values), parent_node, false), is_namespace(is_namespace) {

}

void UsingStmt::declare_and_link(SymbolResolver &linker) {
    chain.declare_and_link(linker);
    auto linked = chain.linked_node();
    if(!linked) {
        linker.error("couldn't find linked node with '" + chain.chain_representation() + "' in using statement");
        return;
    }
    if(is_namespace) {
        auto ns = linked->as_namespace();
        if(ns) {
            for(auto& node : ns->nodes) {
                node->declare_top_level(linker);
            }
        } else {
            linker.error("expected '" + chain.chain_representation() + "' to be a namespace in using statement, however it isn't");
        }
    } else {
        linked->declare_top_level(linker);
    }
}