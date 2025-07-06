// Copyright (c) Chemical Language Foundation 2025.

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

BaseType* EnumMember::known_type() {
    return parent()->known_type();
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

void configure_members_by_inheritance(EnumDeclaration* current, int start) {
    // build sorted list of enum members (sorted by the index specified by the user)
    std::vector<EnumMember*> sorted_members;
    sorted_members.reserve(current->members.size());
    for(auto& member : current->members) {
        sorted_members.emplace_back(member.second);
    }
    std::stable_sort(std::begin(sorted_members), std::end(sorted_members), [](EnumMember* a, EnumMember* b) {
        return a->get_index_dirty() < b->get_index_dirty();
    });
    // now that we have a sorted list of members for current enum
    // we should modify its member index as long as we can predict it
    auto counter = 0;
    for(const auto mem : sorted_members) {
        if(mem->get_index_dirty() == counter) {
            // this means user didn't modify the index
            // we should modify it
            mem->set_index_dirty(start);
            start++;
            counter++;
        } else {
            return;
        }
    }
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
            configure_members_by_inheritance(this, inherited->next_start);
            underlying_integer_type = inherited->underlying_integer_type;
        } else {
            linker.error("given type is not an enum or integer type", encoded_location());
            underlying_integer_type = new (linker.ast_allocator->allocate<IntType>()) IntType();
        }
    }
    linker.scope_start();
    // since members is an unordered map, first we declare all enums
    // then we link their init values
    for(auto& mem : members) {
        linker.declare(mem.first, mem.second);
    }
    // since now all identifiers will be available regardless of order of the map
    for(auto& mem : members) {
        auto& value = mem.second->init_value;
        if(value) {
            value->link(linker, value, nullptr);
        }
    }
    linker.scope_end();
}

EnumDeclaration* EnumDeclaration::get_inherited_enum_decl() {
    const auto inherited = underlying_type->get_direct_linked_node();
    return inherited && inherited->kind() == ASTNodeKind::EnumDecl ? inherited->as_enum_decl_unsafe() : nullptr;
}

BaseType* EnumDeclaration::known_type() {
    return &linked_type;
}
