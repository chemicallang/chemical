#include "AliasStmt.h"
#include "compiler/SymbolResolver.h"
#include "ast/values/AccessChain.h"

void AliasStmt::link_signature(SymbolResolver &linker) {
    const auto stmt = this;

    // currently only identifier values are supported
    if (value->kind() == ValueKind::AccessChain) {
        const auto chain = value->as_access_chain_unsafe();
        if (chain->values.size() != 1 || chain->values.front()->kind() != ValueKind::Identifier) {
            linker.error(stmt) << "incompatible value given to alias";
            return;
        }
    }
    if (value->link(linker, stmt->value, nullptr)) {
        const auto node = value->linked_node();
        if (!node) {
            linker.error(stmt) << "cannot alias incompatible value";
            return;
        }
        if (stmt->specifier >= node->specifier()) {
            linker.error(stmt) << "cannot alias a node to a higher specifier";
            return;
        }
        // declares the node without runtime
        linker.declare_node(stmt->alias_name, node, stmt->specifier, false);
    }
}