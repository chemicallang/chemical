// Copyright (c) Qinetik 2024.

#include "UnsafeBlock.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/FunctionDeclaration.h"

UnsafeBlock::UnsafeBlock(Scope scope) : scope(std::move(scope)) {

}

void UnsafeBlock::declare_and_link(SymbolResolver &linker) {
    const auto func_type = linker.current_func_type;
    if(func_type) {
        const auto func = func_type->as_function();
        if(func) {
            // automatically add unsafe annotation to current function
            func->add_annotation(AnnotationKind::Unsafe);
        }
    }
    auto prev = linker.safe_context;
    linker.safe_context = false;
    scope.link_sequentially(linker);
    linker.safe_context = prev;
}