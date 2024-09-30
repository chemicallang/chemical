// Copyright (c) Qinetik 2024.

#include "PointerType.h"
#include "StringType.h"
#include "LiteralType.h"
#include <memory>
#include "VoidType.h"

const PointerType PointerType::void_ptr_instance((BaseType*) &VoidType::instance, nullptr);

void PointerType::link(SymbolResolver &linker) {
    type->link(linker);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}

bool PointerType::satisfies(BaseType *given) {
    const auto type_kind = type->kind();
    const auto pure = given->pure_type();
    if(type_kind == BaseTypeKind::Char) {
        // this is a char* which is a string
        const auto other = pure->kind();
        if(other == BaseTypeKind::String) {
            return true;
        }
    }
    const auto pointer = pure->pointer_type();
    if(pointer) {
        return type->satisfies(pointer->type);
    }
    return false;
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