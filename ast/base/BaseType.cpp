// Copyright (c) Qinetik 2024.

#include "BaseType.h"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/UnionDef.h"
#include "ast/types/LinkedType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/GenericType.h"
#include <sstream>
#include "ASTNode.h"

std::string BaseType::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

StructDefinition* BaseType::linked_struct_def() {
    const auto linked = linked_node();
    return linked ? linked->as_struct_def() : nullptr;
}

StructDefinition* BaseType::get_generic_struct() {
    auto linked_struct = linked_struct_def();
    if(linked_struct && !linked_struct->generic_params.empty()) {
        return linked_struct;
    } else {
        return nullptr;
    }
}

InterfaceDefinition* BaseType::linked_interface_def() {
    const auto linked = linked_node();
    return linked ? linked->as_interface_def() : nullptr;
}

InterfaceDefinition* BaseType::get_generic_interface() {
    auto linked_interface = linked_interface_def();
    if(linked_interface && !linked_interface->generic_params.empty()) {
        return linked_interface;
    } else {
        return nullptr;
    }
}

InterfaceDefinition* BaseType::linked_dyn_interface() {
    auto pure = pure_type();
    if(pure->kind() == BaseTypeKind::Dynamic) {
        return pure->linked_node()->as_interface_def();
    }
    return nullptr;
}

std::string& BaseType::linked_name() {
    if(kind() == BaseTypeKind::Linked) {
        return ((LinkedType*) (this))->type;
    } else if(kind() == BaseTypeKind::Generic) {
        return ((GenericType*) (this))->referenced->type;
    } else {
#ifdef DEBUG
        throw std::runtime_error("BaseType::linked_name called on unexpected type '" + representation() + "'");
#else
        std::cerr << "BaseType::ref_name called on unexpected type '" + representation() << "'" << std::endl;
        std::string x;
        return x;
#endif
    }
}

MembersContainer* BaseType::get_members_container() {
    const auto direct_node = get_direct_linked_node();
    if(!direct_node) return nullptr;
    switch(direct_node->kind()) {
        case ASTNodeKind::StructDecl:
            return direct_node->as_struct_def_unsafe();
        case ASTNodeKind::VariantDecl:
            return direct_node->as_variant_def_unsafe();
        case ASTNodeKind::UnionDecl:
            return direct_node->as_union_def_unsafe();
        case ASTNodeKind::TypealiasStmt:
            return direct_node->as_typealias_unsafe()->actual_type->get_members_container();
        default:
            return nullptr;
    }
}

FunctionDeclaration* BaseType::get_destructor() {
    auto container = get_members_container();
    return container ? container->destructor_func() : nullptr;
}

FunctionDeclaration* BaseType::get_pre_move_fn() {
    auto container = get_members_container();
    return container ? container->pre_move_func() : nullptr;
}

FunctionDeclaration* BaseType::get_move_fn() {
    auto container = get_members_container();
    return container ? container->move_func() : nullptr;
}

FunctionDeclaration* BaseType::get_clear_fn() {
    auto container = get_members_container();
    return container ? container->clear_func() : nullptr;
}

FunctionDeclaration* BaseType::get_copy_fn() {
    auto container = get_members_container();
    return container ? container->copy_func() : nullptr;
}

bool BaseType::requires_destructor() {
    const auto direct_node = get_direct_linked_node();
    if(!direct_node) return false;
    switch(direct_node->kind()) {
        case ASTNodeKind::StructDecl:
            return direct_node->as_struct_def_unsafe()->requires_destructor();
        case ASTNodeKind::VariantDecl:
            return direct_node->as_variant_def_unsafe()->requires_destructor();
        case ASTNodeKind::UnionDecl:
            return direct_node->as_union_def_unsafe()->requires_destructor();
        case ASTNodeKind::TypealiasStmt:
            return direct_node->as_typealias_unsafe()->actual_type->requires_destructor();
        default:
            return false;
    }
}

bool BaseType::requires_move_fn() {
    const auto direct_node = get_direct_linked_node();
    if(!direct_node) return false;
    switch(direct_node->kind()) {
        case ASTNodeKind::StructDecl:
            return direct_node->as_struct_def_unsafe()->requires_move_fn();
        case ASTNodeKind::VariantDecl:
            return direct_node->as_variant_def_unsafe()->requires_move_fn();
        case ASTNodeKind::UnionDecl:
            return direct_node->as_union_def_unsafe()->requires_move_fn();
        case ASTNodeKind::TypealiasStmt:
            return direct_node->as_typealias_unsafe()->actual_type->requires_move_fn();
        default:
            return false;
    }
}

bool BaseType::requires_clear_fn() {
    const auto direct_node = get_direct_linked_node();
    if(!direct_node) return false;
    switch(direct_node->kind()) {
        case ASTNodeKind::StructDecl:
            return direct_node->as_struct_def_unsafe()->requires_clear_fn();
        case ASTNodeKind::VariantDecl:
            return direct_node->as_variant_def_unsafe()->requires_clear_fn();
        case ASTNodeKind::UnionDecl:
            return direct_node->as_union_def_unsafe()->requires_clear_fn();
        case ASTNodeKind::TypealiasStmt:
            return direct_node->as_typealias_unsafe()->actual_type->requires_clear_fn();
        default:
            return false;
    }
}

bool BaseType::requires_copy_fn() {
    const auto direct_node = get_direct_linked_node();
    if(!direct_node) return false;
    switch(direct_node->kind()) {
        case ASTNodeKind::StructDecl:
            return direct_node->as_struct_def_unsafe()->requires_copy_fn();
        case ASTNodeKind::VariantDecl:
            return direct_node->as_variant_def_unsafe()->requires_copy_fn();
        case ASTNodeKind::UnionDecl:
            return direct_node->as_union_def_unsafe()->requires_copy_fn();
        case ASTNodeKind::TypealiasStmt:
            return direct_node->as_typealias_unsafe()->actual_type->requires_copy_fn();
        default:
            return false;
    }
}

std::unique_ptr<Value> BaseType::promote_unique(Value* value) {
    return std::unique_ptr<Value>(promote(value));
}

bool BaseType::is_reference(BaseTypeKind k) {
    return k == BaseTypeKind::Reference;
}

ASTNode* BaseType::get_direct_linked_node(BaseTypeKind kind) {
    switch(kind) {
        case BaseTypeKind::Linked:
            return ((LinkedType*) this)->linked;
        case BaseTypeKind::Generic:
            return ((GenericType*) this)->referenced->linked;
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union:
            return linked_node();
        default:
            return nullptr;
    }
}

ASTNode* BaseType::get_ref_or_linked_node(BaseTypeKind kind) {
    switch(kind) {
        case BaseTypeKind::Linked:
            return ((LinkedType*) this)->linked;
        case BaseTypeKind::Generic:
            return ((GenericType*) this)->referenced->linked;
        case BaseTypeKind::Reference:
            return ((ReferenceType*) this)->type->get_direct_linked_node();
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union:
            return linked_node();
        default:
            return nullptr;
    }
}

StructDefinition* BaseType::get_direct_linked_struct(BaseTypeKind k) {
    const auto ref_node = get_direct_linked_node(k);
    return ref_node ? ref_node->as_struct_def() : nullptr;
}

StructDefinition* BaseType::get_ref_or_linked_struct(BaseTypeKind k) {
    const auto node = get_ref_or_linked_node(k);
    return node ? node->as_struct_def() : nullptr;
}

VariantDefinition* BaseType::get_direct_linked_variant(BaseTypeKind k) {
    const auto ref_node = get_direct_linked_node(k);
    return ref_node ? ref_node->as_variant_def() : nullptr;
}

StructDefinition* BaseType::get_direct_linked_movable_struct() {
    const auto direct_ref_struct = get_direct_linked_struct();
    if(direct_ref_struct && (direct_ref_struct->destructor_func() || direct_ref_struct->pre_move_func())) {
        return direct_ref_struct;
    } else {
        return nullptr;
    }
}

StructDefinition* BaseType::get_direct_non_movable_struct() {
    const auto direct_ref_struct = get_direct_linked_struct();
    if(direct_ref_struct && direct_ref_struct->pre_move_func() == nullptr && direct_ref_struct->destructor_func() == nullptr) {
        return direct_ref_struct;
    } else {
        return nullptr;
    }
}

bool BaseType::requires_moving(BaseTypeKind k) {
    auto node = get_direct_linked_node(k);
    return node != nullptr && node->requires_moving(node->kind());
}

FunctionDeclaration* BaseType::implicit_constructor_for(Value *value) {
    const auto linked_def = linked_struct_def();
    if(linked_def) {
        const auto prev_itr = linked_def->active_iteration;
        const auto itr = get_generic_iteration();
        if(itr != -1) {
            linked_def->set_active_iteration(itr);
        }
        const auto implicit_constructor = linked_def->implicit_constructor_func(value);
        if(itr != -1) {
            linked_def->set_active_iteration(prev_itr);
        }
        return implicit_constructor;
    }
    return nullptr;
}

int16_t BaseType::set_generic_iteration(int16_t iteration) {
    if(iteration != -1) {
        const auto members_container = linked_node()->as_members_container();
        if (members_container) {
            const auto prev_itr = members_container->active_iteration;
            members_container->set_active_iteration(iteration);
            return prev_itr;
        }
    }
    return -2;
}

BaseType::~BaseType() = default;