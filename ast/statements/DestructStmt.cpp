// Copyright (c) Qinetik 2024.

#include "DestructStmt.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/PointerType.h"
#include "ast/types/ArrayType.h"
#include "ast/structures/StructDefinition.h"

DestructStmt::DestructStmt(
        std::unique_ptr<Value> array_value,
        std::unique_ptr<Value> value,
        bool is_array,
        ASTNode* parent_node
) : array_value(std::move(array_value)), identifier(std::move(value)), is_array(is_array), parent_node(parent_node) {

}

void DestructStmt::declare_and_link(SymbolResolver &linker) {
    identifier->link(linker, (std::unique_ptr<Value>&) identifier);
    auto type = identifier->get_pure_type();
    if(!type->is_pointer()) {
        linker.error("std::delete cannot be called on a value that isn't a pointer");
        return;
    }
    auto found = linker.find("free");
    if(!found || !found->as_function()) {
        linker.error("'free' function should be declared before using std::delete so calls can be made to it");
        return;
    }
    free_func_linked = found->as_function();
}

DestructData DestructStmt::get_data() {

    DestructData data {nullptr, nullptr,  -1 };

    auto pure_type = identifier->get_pure_type();
    bool determined_array = false;
    if(pure_type->kind() == BaseTypeKind::Array) {
        determined_array = true;
    }

    if(!is_array && !determined_array) {
        if(pure_type->kind() != BaseTypeKind::Pointer) {
            return data;
        }
        auto def = ((PointerType*) pure_type.get())->type->get_direct_ref_struct();
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
        auto arr_type = (ArrayType*) pure_type.get();
        int array_size = arr_type->array_size;
        elem_type = arr_type->elem_type->pure_type();
        auto def = elem_type->get_direct_ref_struct();
        if(!def) {
            return data;
        }
        data.parent_node = def;
        data.destructor_func = def->destructor_func();
        data.array_size = array_size;
    } else if(pure_type->kind() == BaseTypeKind::Pointer) {
        if(!array_value) {
            return data;
        }
        auto ptr_type = (PointerType*) pure_type.get();
        elem_type = ptr_type->type->pure_type();
        auto def = ptr_type->type->get_direct_ref_struct();
        if(!def) {
            return data;
        }
        data.parent_node = def;
        data.destructor_func = def->destructor_func();
    }

    return data;

}