// Copyright (c) Chemical Language Foundation 2025.

#include "GenericStructDecl.h"
#include "compiler/SymbolResolver.h"

void GenericStructDecl::declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.declare(master_impl->name_view(), this);
}

void GenericStructDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
//    // symbol resolve the master declaration
//    // TODO this creates an extra scope
//    master_impl->generic_parent = this;
//    linker.scope_start();
//    for(auto& param : generic_params) {
//        param->declare_and_link(linker, (ASTNode*&) param);
//    }
//    master_impl->link_signature_no_scope(linker);
//    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
//    // we set it has usage, so every shallow copy or instantiation has usage
//    master_impl->set_has_usage(true);
//    linker.scope_end();
}