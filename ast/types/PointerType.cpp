// Copyright (c) Qinetik 2024.

#include "PointerType.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type *PointerType::llvm_type(Codegen &gen) const {
    return gen.builder->getPtrTy();
}

#endif


void PointerType::link(SymbolResolver &linker) {
    type->link(linker);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}