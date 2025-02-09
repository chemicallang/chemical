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
    const auto current = type->pure_type();
    const auto type_kind = current->kind();
    const auto other_pure = given->pure_type();
    const auto other_kind = other_pure->kind();
    if(type_kind == BaseTypeKind::Any || (type_kind == BaseTypeKind::IntN && current->as_intn_type_unsafe()->num_bits() == 8)) {
        // this is a char* or uchar* which is a string
        if(other_kind == BaseTypeKind::String) {
            return true;
        }
    }
    if(other_kind == BaseTypeKind::Array) {
        const auto pure_type = ((ArrayType*) other_pure);
        return current->satisfies(pure_type->elem_type);
    }
    const auto pointer = other_pure->pointer_type(other_kind);
    if(pointer && pointer->type) {
        if(!pointer->is_mutable && is_mutable) {
            return false;
        }
        if(type_kind == BaseTypeKind::Void) {
            return true;
        }
        // pointer to integer types must be checked explicitly, *char must not be satisfied by *int
        // because int satisfies char
        const auto other_pointee = pointer->type->pure_type();
        if(type_kind == BaseTypeKind::IntN) {
            const auto other_pointee_kind = other_pointee->kind();
            if(other_pointee_kind == BaseTypeKind::IntN && current->as_intn_type_unsafe()->num_bits() != other_pointee->as_intn_type_unsafe()->num_bits()) {
                return false;
            }
        }
        return current->satisfies(other_pointee);
    }
    return false;
}

bool ReferenceType::satisfies(BaseType* given, Value* value, bool assignment) {
    const auto givenKind = given->kind();
    if(givenKind == BaseTypeKind::Reference) {
        const auto ref = ((ReferenceType*) given);
        return type->satisfies(ref->type) && (!is_mutable || ref->is_mutable);
    }
    // when assigning to a ref, we don't require l value
    if(!assignment && givenKind == BaseTypeKind::IntN) {
        if(value) {
            return value->is_ref_l_value();
        } else {
            return false;
        }
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