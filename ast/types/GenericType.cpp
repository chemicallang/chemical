// Copyright (c) Qinetik 2024.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#endif

void GenericType::link(SymbolResolver &linker) {
    linked = linker.find(base);
    if(!linked) {
        linker.error("unresolved symbol, couldn't find generic type " + base);
    }
}

ASTNode *GenericType::linked_node() {
    return linked;
}