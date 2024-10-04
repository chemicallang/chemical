// Copyright (c) Qinetik 2024.


#include "ProvideStmt.h"
#include "compiler/SymbolResolver.h"

ProvideStmt::ProvideStmt(
    Value* value,
    std::string identifier,
    Scope scope,
    ASTNode* parent,
    CSTToken* token
) : value(value), identifier(std::move(identifier)), body(std::move(scope)), parent_node(parent), token(token) {

}

void ProvideStmt::declare_and_link(SymbolResolver &linker) {
    auto& implicit_args = linker.implicit_args;
    auto found = implicit_args.find(identifier);
    if(found != implicit_args.end()) {
        // record the previous value
        const auto previous = found->second;
        // set the new value
        found->second = value;
        // link the body
        body.link_sequentially(linker);
        // set previous value
        found->second = previous;
    } else {
        // set the value
        implicit_args[identifier] = value;
        // link the body
        body.link_sequentially(linker);
        // remove the value
        implicit_args.erase(identifier);
    }
}
