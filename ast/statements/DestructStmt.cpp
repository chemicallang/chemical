// Copyright (c) Qinetik 2024.

#include "DestructStmt.h"
#include "compiler/SymbolResolver.h"

DestructStmt::DestructStmt(
        std::unique_ptr<Value> array_value,
        std::unique_ptr<Value> value,
        bool is_array,
        ASTNode* parent_node
) : array_value(std::move(array_value)), identifier(std::move(value)), is_array(is_array), parent_node(parent_node) {

}

void DestructStmt::declare_and_link(SymbolResolver &linker) {
    identifier->link(linker, (std::unique_ptr<Value>&) identifier);
    auto type = identifier->get_pure_type();
    if(!type->is_pointer()) {
        linker.error("std::delete cannot be called on a value that isn't a pointer");
        return;
    }
    auto found = linker.find("free");
    if(!found || !found->as_function()) {
        linker.error("'free' function should be declared before using std::delete so calls can be made to it");
        return;
    }
    free_func_linked = found->as_function();
}