// Copyright (c) Chemical Language Foundation 2025.


#include "ComptimeBlock.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/Value.h"

void ComptimeBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    body.link_sequentially(linker);
}
