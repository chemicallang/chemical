// Copyright (c) Qinetik 2024.

#include "PointerType.h"

#include <memory>
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

BaseType* PointerType::pure_type() {
    const auto pure_child = type->pure_type();
    if(pure_child && pure_child != type.get()) {
        auto ptr = new PointerType(hybrid_ptr<BaseType>{ pure_child, false }, token);
        pures.emplace_back(ptr);
        return ptr;
//        pures.emplace_back(std::make_unique<PointerType>(hybrid_ptr<BaseType>{ pure_child, false }, token));
//        return pures.back().get();
    }
    return this;
}