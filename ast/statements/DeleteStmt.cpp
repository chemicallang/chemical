// Copyright (c) Qinetik 2024.

#include "DeleteStmt.h"
#include "compiler/SymbolResolver.h"

DeleteStmt::DeleteStmt(std::unique_ptr<Value> value, bool is_array) : identifier(std::move(value)), is_array(is_array) {

}

void DeleteStmt::declare_and_link(SymbolResolver &linker) {
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