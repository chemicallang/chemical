// Copyright (c) Qinetik 2024.

#include "PointerType.h"

#include <memory>
#include "VoidType.h"

const PointerType PointerType::void_ptr_instance((BaseType*) &VoidType::instance, nullptr);

void PointerType::link(SymbolResolver &linker, BaseType*& current) {
    type->link(linker, type);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}

BaseType* PointerType::pure_type() {
    const auto pure_child = type->pure_type();
    if(pure_child && pure_child != type) {
        auto ptr = new PointerType(pure_child, token);
        pures.emplace_back(ptr);
        return ptr;
//        pures.emplace_back(std::make_unique<PointerType>(hybrid_ptr<BaseType>{ pure_child, false }, token));
//        return pures.back().get();
    }
    return this;
}