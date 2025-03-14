#include "AliasStmt.h"
#include "compiler/SymbolResolver.h"
#include "ast/values/AccessChain.h"

void AliasStmt::declare_top_level(SymbolResolver &linker) {
    // currently only identifier values are supported
    if(value->kind() == ValueKind::AccessChain) {
        const auto chain = value->as_access_chain_unsafe();
        if(chain->values.size() != 1 || chain->values.front()->kind() != ValueKind::Identifier) {
            linker.error(this) << "incompatible value given to alias";
            return;
        }
    }
    if(value->link(linker, value, nullptr)) {
        const auto node = value->linked_node();
        if(!node) {
            linker.error(this) << "cannot alias incompatible value";
            return;
        }
        // declares the node without runtime
        linker.declare_node(alias_name, node, node->specifier(), false);
    }
}