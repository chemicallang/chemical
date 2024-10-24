// Copyright (c) Qinetik 2024.


#include "ProvideStmt.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/Value.h"

ProvideStmt::ProvideStmt(
    Value* value,
    std::string identifier,
    Scope scope,
    ASTNode* parent,
    CSTToken* token
) : value(value), identifier(std::move(identifier)), body(std::move(scope)), parent_node(parent), token(token) {

}

void ProvideStmt::declare_and_link(SymbolResolver &linker) {
    if(value->link(linker, value, nullptr)) {
        put_in(linker.implicit_args, value, &linker, [](ProvideStmt* stmt, void* data) {
            stmt->body.link_sequentially((*(SymbolResolver*) data));
        });
    }
}
