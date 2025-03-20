// Copyright (c) Chemical Language Foundation 2025.

#include "NameMangler.h"
#include "ast/base/ASTNode.h"
#include "ast/base/ExtendableMembersContainerNode.h"
#include "ast/base/LocatedIdentifier.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/ModuleScope.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/Namespace.h"
#include <sstream>

inline void container_name(std::ostream& stream, ExtendableMembersContainerNode* container, const chem::string_view& node_id) {
    if(container->generic_instantiation != -1) {
        stream << node_id;
        stream << "__cgs__";
        stream << container->generic_instantiation;
    } else {
        stream << node_id;
    }
}

void NameMangler::mangle_no_parent(std::ostream& stream, ASTNode* node, const chem::string_view& node_id) {
    switch(node->kind()) {
        case ASTNodeKind::FileScope: {
            // we do not append file scope names
            return;
        }
        case ASTNodeKind::FunctionDecl: {
            const auto decl = node->as_function_unsafe();
            stream << node_id;
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
        case ASTNodeKind::StructDecl: {
            const auto def = node->as_struct_def_unsafe();
            if(!def->is_anonymous()) {
                container_name(stream, def, node_id);
            }
            // anonymous structs have no name
            break;
        }
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::InterfaceDecl:
            container_name(stream, node->as_extendable_members_container_unsafe(), node_id);
            break;
        case ASTNodeKind::NamespaceDecl: {
            const auto ns = node->as_namespace_unsafe();
            if(ns->empty_runtime_name()) {
                // namespace requires empty runtime name, let's check it's scope name is empty and module name is equal to namespace name
                // this is because std module contains std namespace, where we can make namespace anonymous to force function names to be stdstringempty
                // where std is from module name and not namespace name
                const auto parent = ns->parent();
                if(parent && parent->kind() == ASTNodeKind::FileScope) {
                    const auto mod_parent = parent->parent();
                    if(mod_parent && mod_parent->kind() == ASTNodeKind::ModuleScope) {
                        const auto mod = mod_parent->as_module_scope_unsafe();
                        if(mod->scope_name.empty() && mod->module_name == node_id) {
                            return;
                        }
                    }
                }
            }
            stream << node_id;
            break;
        }
        default:
            stream << node_id;
            break;
    }
}

bool NameMangler::mangle_non_func(std::ostream& stream, ASTNode* node) {
    const auto id = node->get_located_id();
    if(id) {
        const auto p = node->parent();
        if (p && p->kind() != ASTNodeKind::FunctionDecl) {
            mangle_non_func(stream, p);
        };
        mangle_no_parent(stream, node, id->identifier);
        return true;
    } else {
        return false;
    }
}

void NameMangler::mangle(std::ostream& stream, FunctionDeclaration* decl) {
    if(decl->is_no_mangle()) {
        stream << decl->name_view();
        return;
    }
    if(decl->parent()) {
        const auto k = decl->parent()->kind();
        switch(k) {
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
            default:
                mangle_non_func(stream, decl->parent());
                break;
        }
    }
    mangle_no_parent(stream, decl, decl->name_view());
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