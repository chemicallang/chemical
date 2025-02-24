// Copyright (c) Chemical Language Foundation 2025.

#include "UnsafeBlock.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionDeclaration.h"

void UnsafeBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto prev = linker.safe_context;
    linker.safe_context = false;
    scope.link_sequentially(linker);
    linker.safe_context = prev;
}