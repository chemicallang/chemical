// Copyright (c) Qinetik 2024.

#include "ReferencedType.h"
#include "ast/statements/Typealias.h"
#include "compiler/SymbolResolver.h"

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
        return linked->create_value_type()->satisfies(value_type);
    };
}

void ReferencedType::link(SymbolResolver &linker) {
    linked = linker.find(type);
    if(!linked) {
        linker.error("unresolved symbol, couldn't find referenced type " + type);
    }
}

ASTNode *ReferencedType::linked_node() {
    return linked;
}