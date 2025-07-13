// Copyright (c) Chemical Language Foundation 2025.

#include "DestructStmt.h"
#include "ast/types/PointerType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructDefinition.h"

DestructData DestructStmt::get_data(ASTAllocator& allocator) {

    DestructData data {nullptr, nullptr,  0 };

    auto created_type = identifier->create_type(allocator);
    auto pure_type = created_type->canonical();
    bool determined_array = false;
    if(pure_type->kind() == BaseTypeKind::Array) {
        determined_array = true;
    }

    if(!is_array && !determined_array) {
        if(pure_type->kind() != BaseTypeKind::Pointer) {
            return data;
        }
        auto def = ((PointerType*) pure_type)->type->canonical()->get_direct_linked_struct();
        if(!def) {
            return data;
        }
        auto destructor = def->destructor_func();
        if(!destructor) {
            return data;
        }
        data.parent_node = def;
        data.destructor_func = destructor;
    }
    if(pure_type->kind() == BaseTypeKind::Array) {
        auto arr_type = (ArrayType*) pure_type;
        const auto elem_type = arr_type->elem_type->canonical();
        auto def = elem_type->get_direct_linked_struct();
        if(!def) {
            return data;
        }
        data.parent_node = def;
        data.destructor_func = def->destructor_func();
        data.array_size = arr_type->get_array_size();
    } else if(pure_type->kind() == BaseTypeKind::Pointer) {
        if(!array_value) {
            return data;
        }
        auto ptr_type = (PointerType*) pure_type;
        auto def = ptr_type->type->canonical()->get_direct_linked_struct();
        if(!def) {
            return data;
        }
        data.parent_node = def;
        data.destructor_func = def->destructor_func();
    }

    return data;

}