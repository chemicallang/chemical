// Copyright (c) Qinetik 2024.

#include "UnsafeBlock.h"
#include "compiler/SymbolResolver.h"

UnsafeBlock::UnsafeBlock(Scope scope) : scope(std::move(scope)) {

}

void UnsafeBlock::declare_and_link(SymbolResolver& linker, ASTNode*& node_ptr) {
    auto prev = linker.safe_context;
    linker.safe_context = false;
    scope.link_sequentially(linker);
    linker.safe_context = prev;
}