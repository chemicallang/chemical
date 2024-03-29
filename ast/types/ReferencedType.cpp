// Copyright (c) Qinetik 2024.

#include "ReferencedType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

#endif

ASTNode* ReferencedType::link(ASTLinker &linker) {
    if(linked) return linked;
    auto found = linker.current.find(type);
    if(found != linker.current.end()) {
        linked = found->second;
        return linked;
    } else {
        linker.error("unresolved symbol, couldn't find type " + type);
        return nullptr;
    }
}