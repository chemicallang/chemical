// Copyright (c) Qinetik 2024.

#include "PointerType.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *PointerType::llvm_type(Codegen &gen) const {
    return type->llvm_type(gen)->getPointerTo();
}

#endif


void PointerType::link(SymbolResolver &linker) {
    type->link(linker);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}