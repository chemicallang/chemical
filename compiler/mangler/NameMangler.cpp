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
#include "ast/values/StructValue.h"
#include "ast/types/IntNType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include <sstream>

inline void container_name(BufferedWriter& stream, ExtendableMembersContainerNode* container) {
    if(container->generic_instantiation != -1) {
        stream << container->name_view();
        stream << "__cgs__";
        stream << container->generic_instantiation;
    } else {
        stream << container->name_view();
    }
}

void NameMangler::mangle_linked(BufferedWriter& stream, StructValue* value) {
    const auto definition = value->linked_extendable();
    if(definition) {
        switch (definition->kind()) {
            case ASTNodeKind::UnionDecl: {
                const auto uni = definition->as_union_def_unsafe();
                mangle(stream, uni);
                return;
            }
            case ASTNodeKind::StructDecl: {
                const auto decl = definition->as_struct_def_unsafe();
                mangle(stream, decl);
                return;
            }
            default:
                return;
        }
    } else {
        const auto refType = value->getRefType();
        switch(refType->kind()) {
            case BaseTypeKind::Struct:
                stream << refType->as_struct_type_unsafe()->name;
                return;
            case BaseTypeKind::Union:
                stream << refType->as_union_type_unsafe()->name;
                return;
            default:
                return;
        }
    }
}

void NameMangler::mangle_no_parent(BufferedWriter& stream, ASTNode* node) {
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

void write_mod_scope(BufferedWriter& stream, ModuleScope* mod) {
    if(!mod->scope_name.empty()) {
        stream << mod->scope_name;
        stream << '_';
    }
    stream << mod->module_name;
    stream << '_';
}

inline void write_file_scope(BufferedWriter& stream, FileScope* p) {
   write_mod_scope(stream, p->parent());
}

inline void write_mangle_parent_of(NameMangler& mangler, BufferedWriter& stream, ASTNode* node) {
    if(!node) return;
    const auto parent = node->parent();
    if(!parent) return;
    switch(parent->kind()) {
        case ASTNodeKind::FileScope: {
            if(!is_node_no_mangle(node)) {
                write_file_scope(stream, parent->as_file_scope_unsafe());
            }
            return;
        }
        case ASTNodeKind::FunctionDecl:
            // local node, no need to mangle
            return;
        default:
            mangler.mangle_non_func(stream, parent);
            return;
    }
}

bool NameMangler::mangle_non_func(BufferedWriter& stream, ASTNode* node) {
    const auto id = node->get_located_id();
    if(id) {
        write_mangle_parent_of(*this, stream, node);
        mangle_no_parent(stream, node);
        return true;
    } else {
        return false;
    }
}

std::string_view to_string(IntNTypeKind kind) {
    switch(kind) {
        case IntNTypeKind::Char:
            return "char";
        case IntNTypeKind::Short:
            return "short";
        case IntNTypeKind::Int:
            return "int";
        case IntNTypeKind::Long:
            return "long";
        case IntNTypeKind::LongLong:
            return "longlong";
        case IntNTypeKind::Int128:
            return "int128";
        case IntNTypeKind::I8:
            return "i8";
        case IntNTypeKind::I16:
            return "i16";
        case IntNTypeKind::I32:
            return "i32";
        case IntNTypeKind::I64:
            return "i64";
        case IntNTypeKind::UChar:
            return "uchar";
        case IntNTypeKind::UShort:
            return "ushort";
        case IntNTypeKind::UInt:
            return "uint";
        case IntNTypeKind::ULong:
            return "ulong";
        case IntNTypeKind::ULongLong:
            return "ulonglong";
        case IntNTypeKind::UInt128:
            return "uint128";
        case IntNTypeKind::U8:
            return "u8";
        case IntNTypeKind::U16:
            return "u16";
        case IntNTypeKind::U32:
            return "u32";
        case IntNTypeKind::U64:
            return "u64";
        default:
            return "";
    }
}

void write_primitive_type(BufferedWriter& stream, BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::IntN:
            stream << to_string(type->as_intn_type_unsafe()->IntNKind());
            stream << '_';
            return;
        case BaseTypeKind::Pointer: {
            const auto ptr_type = type->as_pointer_type_unsafe();
            if(ptr_type->is_mutable) {
                stream << 'm';
            }
            stream << "p_";
            write_primitive_type(stream, ptr_type->type);
            return;
        }
        case BaseTypeKind::Reference: {
            const auto ref_type = type->as_reference_type_unsafe();
            if(ref_type->is_mutable) {
                stream << 'm';
            }
            stream << "r_";
            write_primitive_type(stream, ref_type->type);
            return;
        }
        default:
            return;
    }
}

void NameMangler::mangle_func_parent(BufferedWriter& stream, FunctionDeclaration* decl, ASTNode* parent) {
    switch(parent->kind()) {
        case ASTNodeKind::InterfaceDecl: {
            const auto interface = parent->as_interface_def_unsafe();
            if(interface->is_static()) {
                mangle_non_func(stream, interface);
            } else {
                ExtendableMembersContainerNode* container = (interface->active_user && decl->has_self_param()) ? (ExtendableMembersContainerNode*) interface->active_user : interface;
                mangle_non_func(stream, container);
            };
            break;
        }
        case ASTNodeKind::StructDecl:{
            const auto def = parent->as_struct_def_unsafe();
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
            const auto def = parent->as_impl_def_unsafe();
            if(decl->has_self_param() && def->struct_type) {
                const auto can_node = def->struct_type->get_members_container();
                if(can_node) {
                    mangle_non_func(stream, can_node);
                } else {
                    // since this method is on native types, we need to figure out how to mangle it
                    write_mangle_parent_of(*this, stream, def);
                    write_primitive_type(stream, def->struct_type);
                }
            } else {
                const auto& interface = def->interface_type->get_direct_linked_interface();
                mangle_non_func(stream, interface);
            }
            break;
        }
        case ASTNodeKind::FileScope: {
            if(!decl->is_no_mangle()) {
                write_file_scope(stream, parent->as_file_scope_unsafe());
            }
            break;
        }
        default:
            mangle_non_func(stream, parent);
            break;
    }
}

void NameMangler::mangle_func_parent(BufferedWriter& stream, FunctionDeclaration* decl) {
    const auto parent = decl->parent();
    if(parent) {
        mangle_func_parent(stream, decl, parent);
    }
}

void NameMangler::mangle(BufferedWriter& stream, FunctionDeclaration* decl) {
    if(!decl->is_no_mangle()) {
        mangle_func_parent(stream, decl);
        if(decl->isExtensionFn()) {
            const auto declParent = decl->params[0]->type->linked_node();
            if(declParent) {
                mangle_func_parent(stream, decl, declParent);
            }
        }
    }
    mangle_no_parent(stream, decl);
}

bool NameMangler::mangle(BufferedWriter& stream, ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:
            mangle(stream, node->as_function_unsafe());
            return true;
        default:{
            return mangle_non_func(stream, node);
        }
    }
}

void NameMangler::mangle_vtable_name(BufferedWriter& stream, InterfaceDefinition* interface, StructDefinition* def) {
    mangle(stream, interface);
    mangle(stream, def);
}