// Copyright (c) Qinetik 2024.

#include "BaseType.h"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/UnionDef.h"
#include "ast/types/ReferencedType.h"
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

std::string& BaseType::ref_name() {
    if(kind() == BaseTypeKind::Referenced) {
        return ((ReferencedType*) (this))->type;
    } else if(kind() == BaseTypeKind::Generic) {
        return ((GenericType*) (this))->referenced->type;
    } else {
#ifdef DEBUG
        throw std::runtime_error("BaseType::ref_name called on unexpected type '" + representation() + "'");
#else
        std::cerr << "BaseType::ref_name called on unexpected type '" + representation() << "'" << std::endl;
        std::string x;
        return x;
#endif
    }
}

bool BaseType::requires_destructor() {
    const auto direct_node = get_direct_ref_node();
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
    const auto direct_node = get_direct_ref_node();
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

std::unique_ptr<Value> BaseType::promote_unique(Value* value) {
    return std::unique_ptr<Value>(promote(value));
}

ASTNode* BaseType::get_direct_ref_node(BaseTypeKind k) {
    if(k == BaseTypeKind::Referenced) {
        return ((ReferencedType*) this)->linked;
    } else if(k == BaseTypeKind::Generic) {
        return ((GenericType*) this)->referenced->linked;
    } else {
        return nullptr;
    }
}

StructDefinition* BaseType::get_direct_ref_struct(BaseTypeKind k) {
    const auto ref_node = get_direct_ref_node(k);
    return ref_node ? ref_node->as_struct_def() : nullptr;
}

VariantDefinition* BaseType::get_direct_ref_variant(BaseTypeKind k) {
    const auto ref_node = get_direct_ref_node(k);
    return ref_node ? ref_node->as_variant_def() : nullptr;
}

bool BaseType::is_movable_ref_struct() {
    const auto direct_ref_struct = get_direct_ref_struct();
    if(direct_ref_struct) {
        return direct_ref_struct->requires_destructor() || direct_ref_struct->requires_move_fn();
    } else {
        return false;
    }
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