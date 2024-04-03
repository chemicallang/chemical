// Copyright (c) Qinetik 2024.

#include "ReferencedType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "ast/base/ASTNode.h"

llvm::Type *ReferencedType::llvm_type(Codegen &gen) const {
    if(!linked) return nullptr;
    return linked->llvm_type(gen);
}

#endif

void ReferencedType::link(SymbolResolver &linker) {
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