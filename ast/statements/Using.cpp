// Copyright (c) Chemical Language Foundation 2025.

#include "compiler/SymbolResolver.h"
#include "ast/structures/Namespace.h"
#include "ast/base/Value.h"
#include "UsingStmt.h"

void UsingStmt::declare_symbols(SymbolResolver &linker) {
    auto linked = chain->linked_node();
    if(!linked) {
        linker.error("couldn't find linked node", this);
        return;
    }
    if(is_namespace()) {
        auto ns = linked->as_namespace();
        if(ns) {
            for(auto& node_pair : ns->extended) {
                const auto node = node_pair.second;
                linker.declare(chem::string_view(node_pair.first.data(), node_pair.first.size()), node);
            }
        } else {
            linker.error("expected value to be a namespace, however it isn't", this);
        }
    } else {
        const auto& name_view = linked->get_located_id()->identifier;
        linker.declare(name_view, linked);
    }
}