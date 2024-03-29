// Copyright (c) Qinetik 2024.


#include "GenericType.h"

#ifdef COMPILER_BUILD

#endif

ASTNode* GenericType::link(ASTLinker &linker) {
    auto found = linker.current.find(base);
    if(found != linker.current.end()) {
        linked = found->second;
        return linked;
    } else {
        linker.error("unresolved symbol, couldn't find generic type " + base);
        return nullptr;
    }
}