// Copyright (c) Qinetik 2024.


#include "ComptimeBlock.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/Value.h"

ComptimeBlock::ComptimeBlock(
    Scope scope,
    ASTNode* parent,
    SourceLocation location
) : body(std::move(scope)), parent_node(parent), location(location) {

}

void ComptimeBlock::declare_and_link(SymbolResolver &linker) {
    body.link_sequentially(linker);
}
