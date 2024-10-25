// Copyright (c) Qinetik 2024.

#include "UnsafeBlock.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionDeclaration.h"

UnsafeBlock::UnsafeBlock(Scope scope, SourceLocation location) : scope(std::move(scope)), location(location) {

}

void UnsafeBlock::declare_and_link(SymbolResolver &linker) {
    auto prev = linker.safe_context;
    linker.safe_context = false;
    scope.link_sequentially(linker);
    linker.safe_context = prev;
}