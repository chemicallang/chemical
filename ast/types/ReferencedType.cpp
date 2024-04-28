// Copyright (c) Qinetik 2024.

#include "ReferencedType.h"
#include "ast/statements/Typealias.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "ast/base/ASTNode.h"

llvm::Type *ReferencedType::llvm_type(Codegen &gen) const {
    return linked->llvm_type(gen);
}

#endif

bool ReferencedType::satisfies(ValueType value_type) const {
    if(linked->as_typealias() != nullptr) {
        return ((TypealiasStatement*) linked)->to->satisfies(value_type);
    } else {
        // TODO cannot report an error here can we
        return false;
    };
}

void ReferencedType::link(SymbolResolver &linker) {
    linked = linker.find(type);
    if(!linked) {
        linker.error("unresolved symbol, couldn't find type " + type);
    }
}

ASTNode *ReferencedType::linked_node() {
    return linked;
}