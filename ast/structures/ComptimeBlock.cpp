// Copyright (c) Qinetik 2024.


#include "ComptimeBlock.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/Value.h"

ComptimeBlock::ComptimeBlock(
    Scope scope,
    ASTNode* parent,
    CSTToken* token
) : body(std::move(scope)), parent_node(parent), token(token) {

}

void ComptimeBlock::declare_and_link(SymbolResolver &linker) {
    body.link_sequentially(linker);
}
