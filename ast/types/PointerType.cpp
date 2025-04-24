// Copyright (c) Chemical Language Foundation 2025.

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

bool PointerType::link(SymbolResolver &linker, SourceLocation loc) {
    return type->link(linker, loc);
}

ASTNode *PointerType::linked_node() {
    return type->linked_node();
}

bool PointerType::satisfies(BaseType *given) {
    const auto other_pure = given->canonical();
    const auto other_kind = other_pure->kind();
    if(other_kind == BaseTypeKind::NullPtr) {
        return true;
    }
    const auto current = type->canonical();
    const auto type_kind = current->kind();
    if(type_kind == BaseTypeKind::Any || (type_kind == BaseTypeKind::IntN && current->as_intn_type_unsafe()->is_char_or_uchar_type())) {
        // this is a char* or uchar* which is a string
        if(other_kind == BaseTypeKind::String) {
            return true;
        }
    }
    if(other_kind == BaseTypeKind::Array) {
        const auto pure_type = ((ArrayType*) other_pure);
        return current->satisfies(pure_type->elem_type->canonical());
    }
    if(BaseType::is_pointer(other_kind) && other_pure->as_pointer_type_unsafe()->type) {
        const auto pointer = other_pure->as_pointer_type_unsafe();
        if(!pointer->is_mutable && is_mutable) {
            return false;
        }
        if(type_kind == BaseTypeKind::Void) {
            return true;
        }
        // pointer to integer types must be checked explicitly, *char must not be satisfied by *int
        // because int satisfies char
        const auto other_pointee = pointer->type->canonical();
        if(type_kind == BaseTypeKind::IntN) {
            const auto other_pointee_kind = other_pointee->kind();
            if(other_pointee_kind == BaseTypeKind::IntN && !current->as_intn_type_unsafe()->equals(other_pointee->as_intn_type_unsafe())) {
                return false;
            }
        }
        return current->satisfies(other_pointee);
    }
    return false;
}

bool ReferenceType::satisfies(BaseType* giveNonCan, Value* value, bool assignment) {
    const auto given = giveNonCan->canonical();
    const auto givenKind = given->kind();
    if(givenKind == BaseTypeKind::Reference) {
        const auto ref = ((ReferenceType*) given);
        return type->satisfies(ref->type) && (!is_mutable || ref->is_mutable);
    }
    // when assigning to a ref, we don't require l value
    if(!assignment && givenKind == BaseTypeKind::IntN) {
        if(value) {
            const auto typeSatisfies = type->satisfies(given);
            return is_mutable ? typeSatisfies && value->is_ref_l_value() : typeSatisfies;
        } else {
            return false;
        }
    }
    if(!assignment && is_mutable && !given->is_mutable()) {
        return false;
    }
    return type->satisfies(given);
}

bool ReferenceType::satisfies(ASTAllocator& allocator, Value* value, bool assignment) {
    const auto val_type = value->create_type(allocator);
    return val_type != nullptr && satisfies(val_type, value, assignment);
}