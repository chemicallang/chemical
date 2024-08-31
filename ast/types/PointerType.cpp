// Copyright (c) Qinetik 2024.

#include "PointerType.h"
#include "VoidType.h"

const PointerType PointerType::void_ptr_instance(hybrid_ptr<BaseType> {
    (BaseType*) &VoidType::instance, false
}, nullptr);

void PointerType::link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) {
    std::unique_ptr<BaseType> temp_ptr(type.release());
    temp_ptr->link(linker, temp_ptr);
    type.reset(temp_ptr.release());
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}