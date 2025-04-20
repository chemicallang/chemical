// Copyright (c) Chemical Language Foundation 2025.

#include "NameMangler.h"
#include "ast/base/ASTNode.h"
#include "ast/structures/FileScope.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/base/LocatedIdentifier.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/ModuleScope.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/Namespace.h"
#include <sstream>

inline void container_name(std::ostream& stream, ExtendableMembersContainerNode* container) {
    if(container->generic_instantiation != -1) {
        stream << container->name_view();
        stream << "__cgs__";
        stream << container->generic_instantiation;
    } else {
        stream << container->name_view();
    }
}

void NameMangler::mangle_no_parent(std::ostream& stream, ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl: {
            const auto decl = node->as_function_unsafe();
            stream << decl->name_view();
            if (decl->multi_func_index() != 0) {
                stream << "__cmf_";
                // TODO remove this to_string wrap
                stream << std::to_string(decl->multi_func_index());
            }
            if (decl->generic_instantiation != -1) {
                stream << "__cfg_";
                stream << decl->generic_instantiation;
            }
            break;
        }
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::InterfaceDecl: {
            const auto def = node->as_extendable_members_container_unsafe();
            container_name(stream, def);
            break;
        }
        case ASTNodeKind::NamespaceDecl: {
            const auto ns = node->as_namespace_unsafe();
            if(ns->is_anonymous()) {
                break;
            }
            stream << ns->name();
            break;
        }
        default:
            const auto id = node->get_located_id();
            if(id) {
                stream << id->identifier;
            }
            break;
    }
}

bool is_node_no_mangle(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            return node->as_function_unsafe()->is_no_mangle();
        case ASTNodeKind::StructDecl:
            return node->as_struct_def_unsafe()->is_no_mangle();
        case ASTNodeKind::InterfaceDecl:
            return node->as_interface_def_unsafe()->is_no_mangle();
        case ASTNodeKind::UnionDecl:
            return node->as_union_def_unsafe()->is_no_mangle();
        case ASTNodeKind::VariantDecl:
            return node->as_variant_def_unsafe()->is_no_mangle();
        case ASTNodeKind::TypealiasStmt:
            return node->as_typealias_unsafe()->is_no_mangle();
        case ASTNodeKind::VarInitStmt:
            return node->as_var_init_unsafe()->is_no_mangle();
        default:
            return false;
    }
}

void write_file_scope(std::ostream& stream, ASTNode* node, FileScope* p) {
    const auto scope = p->parent();
    if(scope && !is_node_no_mangle(node)) {
        if(!scope->scope_name.empty()) {
            stream << scope->scope_name;
            stream << '_';
        }
        stream << scope->module_name;
        stream << '_';
    }
}

bool NameMangler::mangle_non_func(std::ostream& stream, ASTNode* node) {
    const auto id = node->get_located_id();
    if(id) {
        const auto p = node->parent();
        if (p) {
            switch(p->kind()) {
                case ASTNodeKind::FileScope: {
                    write_file_scope(stream, node, p->as_file_scope_unsafe());
                    break;
                }
                case ASTNodeKind::FunctionDecl:
                    break;
                default:
                    mangle_non_func(stream, p);
                    break;
            }
        };
        mangle_no_parent(stream, node);
        return true;
    } else {
        return false;
    }
}

void NameMangler::mangle_func_parent(std::ostream& stream, FunctionDeclaration* decl) {
    const auto parent = decl->parent();
    if(parent) {
        switch(parent->kind()) {
            case ASTNodeKind::InterfaceDecl: {
                const auto interface = decl->parent()->as_interface_def_unsafe();
                if(interface->is_static()) {
                    mangle_non_func(stream, interface);
                } else {
                    ExtendableMembersContainerNode* container = (interface->active_user && decl->has_self_param()) ? (ExtendableMembersContainerNode*) interface->active_user : interface;
                    mangle_non_func(stream, container);
                };
                break;
            }
            case ASTNodeKind::StructDecl:{
                const auto def = decl->parent()->as_struct_def_unsafe();
                const auto interface = def->get_overriding_interface(decl);
                if(interface && interface->is_static()) {
                    mangle_non_func(stream, interface);
                } else {
                    ExtendableMembersContainerNode* container = decl->has_self_param() ? def : (interface ? (ExtendableMembersContainerNode*) interface : def);
                    mangle_non_func(stream, container);
                }
                break;
            }
            case ASTNodeKind::ImplDecl: {
                const auto def = decl->parent()->as_impl_def_unsafe();
                if(decl->has_self_param() && def->struct_type) {
                    const auto struct_def = def->struct_type->linked_struct_def();
                    mangle_non_func(stream, struct_def);
                } else {
                    const auto& interface = def->interface_type->linked_interface_def();
                    mangle_non_func(stream, interface);
                }
                break;
            }
            case ASTNodeKind::FileScope: {
                write_file_scope(stream, decl, parent->as_file_scope_unsafe());
                break;
            }
            default:
                mangle_non_func(stream, decl->parent());
                break;
        }
    }
}

void NameMangler::mangle(std::ostream& stream, FunctionDeclaration* decl) {
    if(!decl->is_no_mangle()) {
        mangle_func_parent(stream, decl);
    }
    mangle_no_parent(stream, decl);
}

std::string NameMangler::mangle(FunctionDeclaration* decl) {
    std::stringstream stream;
    mangle(stream, decl);
    return stream.str();
}

bool NameMangler::mangle(std::ostream& stream, ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            mangle(stream, node->as_function_unsafe());
            return true;
        default:{
            return mangle_non_func(stream, node);
        }
    }
}

std::string NameMangler::mangle(ASTNode* node) {
    std::stringstream stream;
    mangle(stream, node);
    return stream.str();
}

void NameMangler::mangle_vtable_name(std::ostream& stream, InterfaceDefinition* interface, StructDefinition* def) {
    mangle(stream, interface);
    mangle(stream, def);
}

std::string NameMangler::mangle_vtable_name(InterfaceDefinition* interface, StructDefinition* def) {
    std::stringstream stream;
    mangle_vtable_name(stream, interface, def);
    return stream.str();
}