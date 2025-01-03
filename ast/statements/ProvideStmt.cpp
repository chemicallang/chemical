// Copyright (c) Qinetik 2024.


#include "ProvideStmt.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/Value.h"

void ProvideStmt::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(value->link(linker, value, nullptr)) {
        put_in(linker.implicit_args, value, &linker, [](ProvideStmt* stmt, void* data) {
            stmt->body.link_sequentially((*(SymbolResolver*) data));
        });
    }
}
