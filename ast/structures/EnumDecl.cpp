// Copyright (c) Qinetik 2024.

#include "EnumDeclaration.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/IntType.h"
#include "ast/types/LinkedType.h"

void EnumMember::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    if(init_value) {
        init_value->link(linker, init_value, nullptr);
    }
    linker.declare(name, this);
}

unsigned int EnumMember::get_default_index() {
    const auto starting_index = parent_node->get_default_starting_index();
    return starting_index + index;
}

BaseType* EnumMember::create_value_type(ASTAllocator& allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(parent_node->name_view(), (ASTNode*) parent_node, encoded_location());
}

BaseType* EnumMember::known_type() {
    return parent_node->known_type();
};

ASTNode *EnumDeclaration::child(const chem::string_view &name) {
    auto mem = members.find(name);
    if(mem == members.end()) {
        const auto inherited = get_inherited_enum_decl();
        return inherited ? inherited->child(name) : nullptr;
    } else {
        return mem->second;
    }
}

void EnumDeclaration::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    underlying_type->link(linker);
    linker.declare_node(name_view(), (ASTNode*) this, specifier(), false);
}

void EnumDeclaration::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    const auto pure_underlying = underlying_type->pure_type(*linker.ast_allocator);
    const auto k = pure_underlying->kind();
    if(k == BaseTypeKind::IntN) {
        underlying_integer_type = pure_underlying->as_intn_type_unsafe();
    } else {
        const auto linked = pure_underlying->get_direct_linked_node();
        if(linked->kind() == ASTNodeKind::EnumDecl) {
            const auto inherited = linked->as_enum_decl_unsafe();
            default_starting_index = inherited->get_default_starting_index() + inherited->members.size();
            underlying_integer_type = inherited->underlying_integer_type;
        } else {
            linker.error("given type is not an enum or integer type", underlying_type);
            underlying_integer_type = new (linker.ast_allocator->allocate<IntType>()) IntType(encoded_location());
        }
    }
    linker.scope_start();
    for(auto& mem : members) {
        mem.second->declare_and_link(linker, (ASTNode*&) mem.second);
    }
    linker.scope_end();
}

EnumDeclaration* EnumDeclaration::get_inherited_enum_decl() {
    const auto inherited = underlying_type->get_direct_linked_node();
    return inherited && inherited->kind() == ASTNodeKind::EnumDecl ? inherited->as_enum_decl_unsafe() : nullptr;
}

BaseType* EnumDeclaration::create_value_type(ASTAllocator& allocator) {
    return new (allocator.allocate<LinkedType>()) LinkedType(name_view(), (ASTNode*) this, encoded_location());
}

BaseType* EnumDeclaration::known_type() {
    return &linked_type;
}
