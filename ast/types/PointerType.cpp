// Copyright (c) Qinetik 2024.

#include "PointerType.h"
#include "StringType.h"
#include "ast/base/Value.h"
#include "LiteralType.h"
#include "ArrayType.h"
#include <memory>
#include "VoidType.h"
#include "ReferenceType.h"
#include "IntNType.h"

const PointerType PointerType::void_ptr_instance((BaseType*) &VoidType::instance, ZERO_LOC);

bool PointerType::link(SymbolResolver &linker) {
    return type->link(linker);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}

bool PointerType::satisfies(BaseType *given) {
    const auto type_kind = type->kind();
    const auto given_pure = given->pure_type();
    const auto other_kind = given_pure->kind();
    if(type_kind == BaseTypeKind::Char) {
        // this is a char* which is a string
        if(other_kind == BaseTypeKind::String) {
            return true;
        }
    }
    if(other_kind == BaseTypeKind::Array) {
        const auto pure_type = ((ArrayType*) given_pure);
        return type->satisfies(pure_type->elem_type);
    }
    const auto pointer = given_pure->pointer_type(other_kind);
    if(pointer && pointer->type) {
        if(!pointer->is_mutable && is_mutable) {
            return false;
        }
        if(type_kind == BaseTypeKind::Void) {
            return true;
        }
        return type->satisfies(pointer->type);
    }
    return false;
}

BaseType* PointerType::pure_type() {
    const auto pure_child = type->pure_type();
    if(pure_child && pure_child != type) {
        // TODO pointer type allocated without an allocator
        auto ptr = new PointerType(pure_child, location, is_mutable);
        pures.emplace_back(ptr);
        return ptr;
//        pures.emplace_back(std::make_unique<PointerType>(hybrid_ptr<BaseType>{ pure_child, false }, token));
//        return pures.back().get();
    }
    return this;
}

bool ReferenceType::satisfies(BaseType* given, Value* value, bool assignment) {
    const auto givenKind = given->kind();
    if(givenKind == BaseTypeKind::Reference) {
        const auto ref = ((ReferenceType*) given);
        return type->satisfies(ref->type) && (!is_mutable || ref->is_mutable);
    }
    // when assigning to a ref, we don't require l value
    if(!assignment && givenKind == BaseTypeKind::IntN) {
        return value->is_ref_l_value();
    }
    if(!assignment && is_mutable && !given->is_mutable(givenKind)) {
        return false;
    }
    return type->satisfies(given);
}

bool ReferenceType::satisfies(ASTAllocator& allocator, Value* value, bool assignment) {
    const auto val_type = value->create_type(allocator);
    return val_type != nullptr && satisfies(val_type->pure_type(), value, assignment);
}