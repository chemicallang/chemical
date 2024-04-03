// Copyright (c) Qinetik 2024.


#include "GenericType.h"

#ifdef COMPILER_BUILD

#endif

void GenericType::link(SymbolResolver &linker) {
    auto found = linker.current.find(base);
    if(found != linker.current.end()) {
        linked = found->second;
    } else {
        linker.error("unresolved symbol, couldn't find generic type " + base);
    }
}

ASTNode *GenericType::linked_node() {
    return linked;
}