// Copyright (c) Qinetik 2024.

#include "ReferencedType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

#endif

void ReferencedType::link(ASTLinker &linker) {
    auto found = linker.current.find(type);
    if(found != linker.current.end()) {
        linked = found->second;
    } else {
        linker.error("unresolved symbol, couldn't find type " + type);
    }
}

ASTNode *ReferencedType::linked_node() {
    return linked;
}