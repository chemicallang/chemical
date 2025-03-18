// Copyright (c) Chemical Language Foundation 2025.

#include "ASTNode.h"
#include "ASTUnit.h"
#include "BaseType.h"
#include "Value.h"
#include "ast/structures/VariantDefinition.h"
#include "compiler/Codegen.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/Namespace.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/If.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/InterfaceDefinition.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

#if !defined(DEBUG) && defined(COMPILER_BUILD)
#include "compiler/Codegen.h"
#endif

LocatedIdentifier ZERO_LOC_ID(BatchAllocator& allocator, std::string& identifier) {
    const auto size = identifier.size();
    const auto ptr = allocator.allocate_str(identifier.data(), size);
#ifdef LSP_BUILD
    return { chem::string_view(ptr, size), ZERO_LOC };
#else
    return { chem::string_view(ptr, size) };
#endif
}

//LocatedIdentifier LOC_ID(BatchAllocator& allocator, std::string identifier, SourceLocation location) {
//    const auto size = identifier.size();
//    const auto ptr = allocate(allocator, identifier.data(), size);
//#ifdef LSP_BUILD
//    return { chem::string_view(ptr, size), location };
//#else
//    return { chem::string_view(ptr, size) };
//#endif
//}

std::string ASTNode::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    visitor.visit(this);
    return ostring.str();
}

MembersContainer* ASTNode::get_members_container() {
    const auto k = kind();
    if(isMembersContainer(k)) {
        return (MembersContainer*) this;
    } else if(k == ASTNodeKind::TypealiasStmt) {
        return ((TypealiasStatement*) this)->actual_type->get_members_container();
    }
    return nullptr;
}

void ASTNode::runtime_name(std::ostream& stream) {
    const auto p = parent();
    if(p && p->kind() != ASTNodeKind::FunctionDecl) p->runtime_name(stream);
    stream << get_located_id()->identifier;
}

void ASTNode::runtime_name_no_parent(std::ostream& stream) {
    stream << get_located_id()->identifier;
}

std::string ASTNode::runtime_name_str() {
    std::stringstream stream;
    runtime_name(stream);
    return stream.str();
}

LocatedIdentifier* ASTNode::get_located_id() {
    switch(kind()) {
        case ASTNodeKind::VarInitStmt:
            return as_var_init_unsafe()->get_located_id();
        case ASTNodeKind::TypealiasStmt:
            return as_typealias_unsafe()->get_located_id();
        case ASTNodeKind::EnumDecl:
            return as_enum_decl_unsafe()->get_located_id();
        case ASTNodeKind::FunctionDecl:
            return as_function_unsafe()->get_located_id();
        case ASTNodeKind::GenericFuncDecl:
            return as_gen_func_decl_unsafe()->master_impl->get_located_id();
        case ASTNodeKind::InterfaceDecl:
            return as_interface_def_unsafe()->get_located_id();
        case ASTNodeKind::GenericStructDecl:
            return as_gen_struct_def_unsafe()->master_impl->get_located_id();
        case ASTNodeKind::GenericUnionDecl:
            return as_gen_union_decl_unsafe()->master_impl->get_located_id();
        case ASTNodeKind::GenericInterfaceDecl:
            return as_gen_interface_decl_unsafe()->master_impl->get_located_id();
        case ASTNodeKind::GenericVariantDecl:
            return as_gen_variant_decl_unsafe()->master_impl->get_located_id();
        case ASTNodeKind::StructDecl:
            return as_struct_def_unsafe()->get_located_id();
        case ASTNodeKind::UnionDecl:
            return as_union_def_unsafe()->get_located_id();
        case ASTNodeKind::VariantDecl:
            return as_variant_def_unsafe()->get_located_id();
        case ASTNodeKind::NamespaceDecl:
            return as_namespace_unsafe()->get_located_id();
        default:
            return nullptr;
    }
}

BaseType* ASTNode::known_type_SymRes(ASTAllocator& allocator) {
    if(kind() == ASTNodeKind::VarInitStmt) {
        return as_var_init_unsafe()->known_type_SymRes(allocator);
    } else {
        return known_type();
    }
}

uint64_t ASTNode::byte_size(bool is64Bit) {
    auto holdingType = known_type();
    if(holdingType) return holdingType->byte_size(is64Bit);
    auto holdingValue = holding_value();
    if(holdingValue) return holdingValue->byte_size(is64Bit);
    throw std::runtime_error("unknown byte size for linked node");
}

ASTNode* ASTNode::root_parent() {
    ASTNode* current = this;
    while(true) {
        const auto parent = current->parent();
        if(parent) {
            current = parent;
        } else {
            return current;
        }
    };
}

AccessSpecifier ASTNode::specifier() {
    const auto k = kind();
    switch(k) {
        case ASTNodeKind::StructDecl:
            return as_struct_def_unsafe()->specifier();
        case ASTNodeKind::GenericFuncDecl:
            return as_gen_func_decl_unsafe()->master_impl->specifier();
        case ASTNodeKind::GenericStructDecl:
            return as_gen_struct_def_unsafe()->master_impl->specifier();
        case ASTNodeKind::GenericUnionDecl:
            return as_gen_union_decl_unsafe()->master_impl->specifier();
        case ASTNodeKind::GenericInterfaceDecl:
            return as_gen_interface_decl_unsafe()->master_impl->specifier();
        case ASTNodeKind::GenericVariantDecl:
            return as_gen_variant_decl_unsafe()->master_impl->specifier();
        case ASTNodeKind::VariantDecl:
            return as_variant_def_unsafe()->specifier();
        case ASTNodeKind::NamespaceDecl:
            return as_namespace_unsafe()->specifier();
        case ASTNodeKind::UnionDecl:
            return as_union_def_unsafe()->specifier();
        case ASTNodeKind::EnumDecl:
            return as_enum_decl_unsafe()->specifier();
        case ASTNodeKind::FunctionDecl:
            return as_function_unsafe()->specifier();
        case ASTNodeKind::InterfaceDecl:
            return as_interface_def_unsafe()->specifier();
        case ASTNodeKind::VarInitStmt:
            return as_var_init_unsafe()->specifier();
        case ASTNodeKind::TypealiasStmt:
            return as_typealias_unsafe()->specifier();
        default:
            return AccessSpecifier::Private;
    }
}

bool ASTNode::set_deprecated(bool value) {
    switch(kind()) {
        case ASTNodeKind::StructDecl:
            as_struct_def_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::FunctionDecl:
            as_function_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::VariantDecl:
            as_variant_def_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::UnionDecl:
            as_union_def_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::InterfaceDecl:
            as_interface_def_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::GenericFuncDecl:
            as_gen_func_decl_unsafe()->master_impl->set_deprecated(value);
            return true;
        case ASTNodeKind::GenericStructDecl:
            as_gen_struct_def_unsafe()->master_impl->set_deprecated(value);
            return true;
        case ASTNodeKind::GenericUnionDecl:
            as_gen_union_decl_unsafe()->master_impl->set_deprecated(value);
            return true;
        case ASTNodeKind::GenericInterfaceDecl:
            as_gen_interface_decl_unsafe()->master_impl->set_deprecated(value);
            return true;
        case ASTNodeKind::GenericVariantDecl:
            as_gen_variant_decl_unsafe()->master_impl->set_deprecated(value);
            return true;
        case ASTNodeKind::TypealiasStmt:
            as_typealias_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::EnumDecl:
            as_enum_decl_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::VarInitStmt:
            as_var_init_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::EnumMember:
            as_enum_member_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::StructMember:
            as_struct_member_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::NamespaceDecl:
            as_namespace_unsafe()->set_deprecated(value);
            return true;
        case ASTNodeKind::VariantMember:
            as_variant_member_unsafe()->set_deprecated(value);
            return true;
        default:
            return false;
    }
}

bool ASTNode::set_anonymous(bool value) {
    switch(kind()) {
        case ASTNodeKind::StructDecl:
            as_struct_def_unsafe()->set_anonymous(value);
            return true;
        case ASTNodeKind::UnionDecl:
            as_union_def_unsafe()->set_anonymous(value);
            return true;
        case ASTNodeKind::VariantDecl:
            as_variant_def_unsafe()->set_anonymous(value);
            return true;
        case ASTNodeKind::GenericStructDecl:
            as_gen_struct_def_unsafe()->master_impl->set_anonymous(value);
            return true;
        case ASTNodeKind::GenericUnionDecl:
            as_gen_union_decl_unsafe()->master_impl->set_anonymous(value);
            return true;
        case ASTNodeKind::GenericInterfaceDecl:
            as_gen_interface_decl_unsafe()->master_impl->set_anonymous(value);
            return true;
        case ASTNodeKind::GenericVariantDecl:
            as_gen_variant_decl_unsafe()->master_impl->set_anonymous(value);
            return true;
        default:
            return false;
    }
}

bool ASTNode::is_shallow_copyable() {
    switch(kind()) {
        case ASTNodeKind::StructDecl:
            return as_struct_def_unsafe()->is_shallow_copyable();
        case ASTNodeKind::UnionDecl:
            return as_union_def_unsafe()->is_shallow_copyable();
        case ASTNodeKind::VariantDecl:
            return as_variant_def_unsafe()->is_shallow_copyable();
        default:
            return false;
    }
}

void ASTNode::set_shallow_copyable(bool value) {
    switch(kind()) {
        case ASTNodeKind::StructDecl:
            as_struct_def_unsafe()->set_shallow_copyable(value);
            return;
        case ASTNodeKind::UnionDecl:
            as_union_def_unsafe()->set_shallow_copyable(value);
            return;
        case ASTNodeKind::VariantDecl:
            as_variant_def_unsafe()->set_shallow_copyable(value);
            return;
        default:
            return;
    }
}

bool ASTNode::set_specifier(AccessSpecifier spec) {
    const auto k = kind();
    switch(k) {
        case ASTNodeKind::StructDecl:
            as_struct_def_unsafe()->set_specifier(spec);
            return true;
        case ASTNodeKind::VariantDecl:
            as_variant_def_unsafe()->set_specifier(spec);
            return true;
        case ASTNodeKind::NamespaceDecl:
            as_namespace_unsafe()->set_specifier(spec);
            return true;
        case ASTNodeKind::UnionDecl:
            as_union_def_unsafe()->set_specifier(spec);
            return true;
        case ASTNodeKind::EnumDecl:
            as_enum_decl_unsafe()->set_specifier(spec);
            return true;
        case ASTNodeKind::FunctionDecl:
            as_function_unsafe()->set_specifier_fast(spec);
            return true;
        case ASTNodeKind::InterfaceDecl:
            as_interface_def_unsafe()->set_specifier_fast(spec);
            return true;
        case ASTNodeKind::GenericFuncDecl:
            as_gen_func_decl_unsafe()->master_impl->set_specifier_fast(spec);
            return true;
        case ASTNodeKind::GenericStructDecl:
            as_gen_struct_def_unsafe()->master_impl->set_specifier(spec);
            return true;
        case ASTNodeKind::GenericUnionDecl:
            as_gen_union_decl_unsafe()->master_impl->set_specifier(spec);
            return true;
        case ASTNodeKind::GenericInterfaceDecl:
            as_gen_interface_decl_unsafe()->master_impl->set_specifier_fast(spec);
            return true;
        case ASTNodeKind::GenericVariantDecl:
            as_gen_variant_decl_unsafe()->master_impl->set_specifier(spec);
            return true;
        default:
            return false;
    }
}

bool ASTNode::is_exported() {
    return specifier() == AccessSpecifier::Public;
}

bool ASTNode::set_comptime(bool value) {
    switch(kind()) {
        case ASTNodeKind::VarInitStmt:
            as_var_init_unsafe()->set_comptime(value);
            return true;
        case ASTNodeKind::StructDecl:
            as_struct_def_unsafe()->set_comptime(value);
            return true;
        case ASTNodeKind::NamespaceDecl:
            as_namespace_unsafe()->set_comptime(value);
            return true;
        case ASTNodeKind::FunctionDecl:
            as_function_unsafe()->set_comptime(value);
            return true;
        case ASTNodeKind::TypealiasStmt:
            as_typealias_unsafe()->set_comptime(value);
            return true;
        case ASTNodeKind::GenericFuncDecl:
            as_gen_func_decl_unsafe()->master_impl->set_comptime(value);
            return true;
        case ASTNodeKind::GenericStructDecl:
            as_gen_struct_def_unsafe()->master_impl->set_comptime(value);
            return true;
        case ASTNodeKind::GenericUnionDecl:
            as_gen_union_decl_unsafe()->master_impl->set_comptime(value);
            return true;
        case ASTNodeKind::GenericInterfaceDecl:
            as_gen_interface_decl_unsafe()->master_impl->set_comptime(value);
            return true;
        case ASTNodeKind::GenericVariantDecl:
            as_gen_variant_decl_unsafe()->master_impl->set_comptime(value);
            return true;
        default:
            return false;
    }
}

BaseType* ASTNode::get_stored_value_type(ASTAllocator& allocator, ASTNodeKind k) {
    switch(k) {
        case ASTNodeKind::StructMember:
            return as_struct_member_unsafe()->type;
        case ASTNodeKind::VariantCaseVariable:
            return as_variant_case_var_unsafe()->member_param->type;
        case ASTNodeKind::VarInitStmt: {
            const auto init = as_var_init_unsafe();
            if(init->is_const()) {
                return nullptr;
            }
            if (init->type) {
                return init->type;
            } else {
                return init->value->create_type(allocator);
            }
        }
        default:
            return nullptr;
    }
}

bool ASTNode::is_stored_ptr_or_ref(ASTAllocator& allocator, ASTNodeKind k) {
    const auto type = get_stored_value_type(allocator, k);
    return type != nullptr && type->is_pointer_or_ref();
}

bool ASTNode::is_ptr_or_ref(ASTAllocator& allocator, ASTNodeKind k) {
    switch(k) {
        case ASTNodeKind::FunctionParam:
            return as_func_param_unsafe()->type->is_pointer_or_ref();
        default:
            return is_stored_ptr_or_ref(allocator, k);
    }
}

bool ASTNode::is_stored_ref(ASTAllocator& allocator) {
    const auto k = kind();
    switch(k) {
        case ASTNodeKind::FunctionParam:
            return false;
        default:
            const auto type = get_stored_value_type(allocator, k);
            return type != nullptr && type->is_reference();
    }
}

bool ASTNode::is_ref(ASTAllocator& allocator) {
    const auto k = kind();
    switch(k) {
        case ASTNodeKind::FunctionParam:
            return as_func_param_unsafe()->type->is_reference();
        default:
            const auto type = get_stored_value_type(allocator, k);
            return type != nullptr && type->is_reference();
    }
}

bool ASTNode::requires_moving(ASTNodeKind k) {
    switch(k) {
        case ASTNodeKind::StructDecl:
            return as_struct_def_unsafe()->has_destructor();
        case ASTNodeKind::VariantDecl:
            return as_variant_def_unsafe()->requires_destructor();
        default:
            return false;
    }
}

//bool ASTNode::has_moved(ASTNodeKind k, Value* ref) {
//    switch(k) {
//        case ASTNodeKind::FunctionParam:
//            return as_func_param_unsafe()->get_has_moved();
//        case ASTNodeKind::VarInitStmt:
//            return as_var_init_unsafe()->get_has_moved();
//        default:
//            return false;
//    }
//}
//
//void ASTNode::set_moved(ASTNodeKind k, Value* ref) {
//    switch(k) {
//        case ASTNodeKind::VarInitStmt:
//            as_var_init_unsafe()->moved();
//            return;
//        case ASTNodeKind::FunctionParam:
//            as_func_param_unsafe()->moved();
//            return;
//        default:
//            return;
//    }
//}

#ifdef COMPILER_BUILD

llvm::Value *ASTNode::llvm_pointer(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_pointer called on bare ASTNode, with representation" + representation());
#else
    std::cerr << ("ASTNode::llvm_pointer called, on node : " + representation());
    return nullptr;
#endif
};

void ASTNode::code_gen(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("ASTNode code_gen called on bare ASTNode, with representation : " + representation());
#else
    std::cerr << ("ASTNode::code_gen called, on node : " + representation());
#endif
}

bool ASTNode::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
#ifdef DEBUG
    throw std::runtime_error("add_child_index called on a ASTNode");
#else
    std::cerr << ("ASTNode::add_child_index called, on node : " + representation());
    return false;
#endif
}

llvm::Value *ASTNode::llvm_load(Codegen& gen, SourceLocation location) {
#ifdef DEBUG
    throw std::runtime_error("llvm_load called on a ASTNode");
#else
    std::cerr << ("ASTNode::llvm_load called, on node : " + representation());
    return nullptr;
#endif
}

void ASTNode::llvm_destruct(Codegen& gen, llvm::Value* allocaInst, SourceLocation location) {
    switch(kind()) {
        case ASTNodeKind::StructDecl:
            as_struct_def_unsafe()->llvm_destruct(gen, allocaInst, location);
            return;
        case ASTNodeKind::VariantDecl:
            as_variant_def_unsafe()->llvm_destruct(gen, allocaInst, location);
            return;
        case ASTNodeKind::VariantMember:
            // TODO find out, if we allocated exactly the variant member and we need to de allocate that
            as_variant_member_unsafe()->parent()->llvm_destruct(gen, allocaInst, location);
            return;
        case ASTNodeKind::TypealiasStmt:{
            const auto linked = as_typealias_unsafe()->actual_type->get_direct_linked_node();
            if(linked) {
                linked->llvm_destruct(gen, allocaInst, location);
            }
            return;
        }
        default:
            return;
    }
}

#endif

ASTNode::~ASTNode() = default;

ASTUnit::ASTUnit() : scope(nullptr, ZERO_LOC) {}

ASTUnit::ASTUnit(ASTUnit&& other) noexcept = default;

ASTUnit& ASTUnit::operator =(ASTUnit&& other) noexcept = default;

ASTUnit::~ASTUnit() = default;