// Copyright (c) Qinetik 2024.

#include "DestructStmt.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/PointerType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructDefinition.h"

void DestructStmt::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(array_value) {
        array_value->link(linker, array_value);
    }
    if(!identifier->link(linker, identifier)) {
        return;
    }
    auto type = identifier->get_pure_type(linker.allocator);
    if(!type->is_pointer()) {
        linker.error("destruct cannot be called on a value that isn't a pointer", this);
        return;
    }
    auto found = linker.find("free");
    if(!found || !found->as_function()) {
        linker.error("'free' function should be declared before using destruct so calls can be made to it", this);
        return;
    }
    free_func_linked = found->as_function();
}

DestructData DestructStmt::get_data(ASTAllocator& allocator) {

    DestructData data {nullptr, nullptr,  0 };

    auto created_type = identifier->create_type(allocator);
    auto pure_type = created_type->pure_type();
//    auto pure_type = identifier->get_pure_type();
    bool determined_array = false;
    if(pure_type->kind() == BaseTypeKind::Array) {
        determined_array = true;
    }

    if(!is_array && !determined_array) {
        if(pure_type->kind() != BaseTypeKind::Pointer) {
            return data;
        }
        auto def = ((PointerType*) pure_type)->type->pure_type()->get_direct_linked_struct();
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
    BaseType* elem_type;
    if(pure_type->kind() == BaseTypeKind::Array) {
        auto arr_type = (ArrayType*) pure_type;
        elem_type = arr_type->elem_type->pure_type();
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
        elem_type = ptr_type->type->pure_type();
        auto def = ptr_type->type->pure_type()->get_direct_linked_struct();
        if(!def) {
            return data;
        }
        data.parent_node = def;
        data.destructor_func = def->destructor_func();
    }

    return data;

}