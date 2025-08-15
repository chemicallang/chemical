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
#include "ast/values/FunctionCall.h"
#include "ast/values/StringValue.h"
#include "compiler/mangler/NameMangler.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/statements/EmbeddedNode.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/values/PatternMatchExpr.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Import.h"
#include "ast/structures/StructMember.h"
#include "ast/types/PointerType.h"
#include "ast/types/StructType.h"
#include "ast/types/UnionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/BoolType.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/If.h"
#include "ast/values/AddrOfValue.h"
#include "ast/statements/Return.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/BoolValue.h"
#include "ast/values/NullValue.h"
#include "preprocess/RepresentationVisitor.h"
#include "compiler/lab/LabGetMethodInjection.h"
#include <sstream>
#include <iostream>

#if !defined(DEBUG) && defined(COMPILER_BUILD)
#include "compiler/Codegen.h"
#endif

LocatedIdentifier ZERO_LOC_ID(BatchAllocator& allocator, std::string& identifier) {
    const auto size = identifier.size();
    const auto ptr = allocator.allocate_str(identifier.data(), size);
#ifdef LSP_BUILD
    return { chem::string_view(ptr, size) };
#else
    return { chem::string_view(ptr, size) };
#endif
}

VarInitStatement* default_build_lab_build_flag(ASTAllocator& allocator, TypeBuilder& builder, ASTNode* parent) {
    const auto buildFlagValue = new (allocator.allocate<BoolValue>()) BoolValue(true, builder.getBoolType(), ZERO_LOC);
    const auto stmt = new (allocator.allocate<VarInitStatement>()) VarInitStatement(false, false, LocatedIdentifier(chem::string_view("__chx_should_build")), {builder.getBoolType(), ZERO_LOC}, buildFlagValue, parent, ZERO_LOC);
    return stmt;
}

VarInitStatement* default_build_lab_cached_ptr(ASTAllocator& allocator, TypeBuilder& builder, ASTNode* parent) {

    // the type for the *Module
    const auto modNmdType = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(chem::string_view("Module"));
    const auto ptrModNmdType = new (allocator.allocate<PointerType>()) PointerType(modNmdType, true);

    const auto buildPtrValue = new (allocator.allocate<NullValue>()) NullValue(builder.getNullPtrType(), ZERO_LOC);
    const auto stmt = new (allocator.allocate<VarInitStatement>()) VarInitStatement(false, false, LocatedIdentifier(chem::string_view("__chx_cached_build")), {ptrModNmdType, ZERO_LOC}, buildPtrValue, parent, ZERO_LOC);
    return stmt;
}

FunctionDeclaration* default_build_lab_get_method(ASTAllocator& allocator, TypeBuilder& builder, ASTNode* parent, const chem::string_view& buildFlagName, const chem::string_view& cachedPtrName) {

    // the type for the *Module
    const auto modNmdType = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(chem::string_view("Module"));
    const auto ptrModNmdType = new (allocator.allocate<PointerType>()) PointerType(modNmdType, true);

    // creating the function decl
    auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(LocatedIdentifier(chem::string_view("get")), {ptrModNmdType, ZERO_LOC}, false, parent, ZERO_LOC, AccessSpecifier::Public, false);

    // the type for the *BuildContext
    const auto buildContextNmdType = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(chem::string_view("BuildContext"));
    const auto ptrBuildCtx = new (allocator.allocate<PointerType>()) PointerType(buildContextNmdType, true);

    // the context parameter
    const auto ctxParam = new (allocator.allocate<FunctionParam>()) FunctionParam(chem::string_view("ctx"), TypeLoc(ptrBuildCtx, ZERO_LOC), 0, nullptr, false, decl, ZERO_LOC);
    decl->params.emplace_back(ctxParam);

    // lets do function body
    decl->body.emplace(decl, ZERO_LOC);
    const auto ctxId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(chem::string_view("ctx"), ptrBuildCtx, ZERO_LOC, false);
    const auto defGetId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(chem::string_view("default_get"), nullptr, ZERO_LOC, false);
    const auto callParentChain = new (allocator.allocate<AccessChain>()) AccessChain({ ctxId, defGetId }, ZERO_LOC);
    const auto funcCall = new (allocator.allocate<FunctionCall>()) FunctionCall(callParentChain, ZERO_LOC);

    // build flag id argument
    const auto buildFlagId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(buildFlagName, nullptr, ZERO_LOC, false);
    const auto buildFlagIdWrap = new (allocator.allocate<AccessChain>()) AccessChain({ buildFlagId }, nullptr, ZERO_LOC);
    const auto buildFlagPtr = new (allocator.allocate<AddrOfValue>()) AddrOfValue(buildFlagIdWrap, true, ZERO_LOC);

    // cached ptr id argument
    const auto cachedPtrId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(cachedPtrName, nullptr, ZERO_LOC, false);
    const auto cachedPtrIdWrap = new (allocator.allocate<AccessChain>()) AccessChain({ cachedPtrId }, nullptr, ZERO_LOC);
    const auto cachedPtrPtr = new (allocator.allocate<AddrOfValue>()) AddrOfValue(cachedPtrIdWrap, true, ZERO_LOC);

    // lets do default get function call arguments
    const auto buildFuncId = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(chem::string_view("build"), nullptr, ZERO_LOC, false);
    const auto buildFuncIdWrap = new (allocator.allocate<AccessChain>()) AccessChain({ buildFuncId }, nullptr, ZERO_LOC);

    // putting arguments
    funcCall->values = { buildFlagPtr, cachedPtrPtr, buildFuncIdWrap };

    // just wrapping the function call in a chain before return
    const auto funcCallWrap = new (allocator.allocate<AccessChain>()) AccessChain({ funcCall }, nullptr, ZERO_LOC);
    const auto retStmt = new (allocator.allocate<ReturnStatement>()) ReturnStatement(funcCallWrap, decl, ZERO_LOC);
    decl->body.value().nodes.emplace_back(retStmt);

    // returning the created declaration
    return decl;
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

bool ASTNode::is_top_level() {
    if(!parent()) return true;
    switch(parent()->kind()) {
        case ASTNodeKind::FileScope:
        case ASTNodeKind::NamespaceDecl:
            return true;
        default:
            return false;
    }
}

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
        case ASTNodeKind::GenericTypeDecl:
            return as_gen_type_decl_unsafe()->master_impl->get_located_id();
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

ASTNode* ASTNode::get_ancestor_by_kind(ASTNodeKind k) {
    if(kind() == k) {
        return this;
    } else {
        const auto p = parent();
        return p ? p->get_ancestor_by_kind(k) : nullptr;
    }
}

AccessSpecifier ASTNode::specifier() {
    const auto k = kind();
    switch(k) {
        case ASTNodeKind::StructDecl:
            return as_struct_def_unsafe()->specifier();
        case ASTNodeKind::GenericTypeDecl:
            return as_gen_type_decl_unsafe()->master_impl->specifier();
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

ASTNode* child(ImportStatement* stmt, const chem::string_view &name) {
    if(stmt->symbols) {
        auto found = stmt->symbols->find(name);
        return found != stmt->symbols->end() ? found->second : nullptr;
    } else {
#ifdef DEBUG
        throw std::runtime_error("symbols pointer doesn't exist in import statement");
#endif
        return nullptr;
    }
}

ASTNode* child(VariablesContainer* container, const chem::string_view& name) {
    auto found = container->indexes.find(name);
    return found != container->indexes.end() ? found->second : nullptr;
}

ASTNode* direct_child_provider(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Linked: {
            const auto linked = type->as_linked_type_unsafe()->linked;
            if(linked->kind() == ASTNodeKind::TypealiasStmt) {
                return direct_child_provider(linked->as_typealias_unsafe()->actual_type);
            } else {
                return linked;
            }
        }
        case BaseTypeKind::Generic:
            return direct_child_provider(type->as_generic_type_unsafe()->referenced);
        default:
            return nullptr;
    }
}

// *dyn Phone <--- allowed ? NO (needs a dereference)
// dyn *Phone <--- allowed ? NO
// dyn Phone <---- allowed ? YES
ASTNode* child_provider(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Pointer:
            return direct_child_provider(type->as_pointer_type_unsafe()->type);
        case BaseTypeKind::Reference:
            return direct_child_provider(type->as_reference_type_unsafe()->type);
        case BaseTypeKind::Linked: {
            const auto linked = type->as_linked_type_unsafe()->linked;
            if(linked->kind() == ASTNodeKind::TypealiasStmt) {
                return child_provider(linked->as_typealias_unsafe()->actual_type);
            } else {
                return linked;
            }
        }
        case BaseTypeKind::Generic:
            return child_provider(type->as_generic_type_unsafe()->referenced);
        case BaseTypeKind::Dynamic:
            return direct_child_provider(type->as_dynamic_type_unsafe()->referenced);
        case BaseTypeKind::Struct:
            return type->as_struct_type_unsafe();
        case BaseTypeKind::Union:
            return type->as_union_type_unsafe();
        case BaseTypeKind::Function:
            return child_provider(type->as_function_type_unsafe()->returnType);
        case BaseTypeKind::CapturingFunction:
            return child_provider(type->as_capturing_func_type_unsafe()->func_type);
        default:
            return nullptr;
    }
}

ASTNode* provide_child(BaseType* type, const chem::string_view& name) {
    const auto provider = child_provider(type);
    return provider ? provider->child(name) : nullptr;
}

ASTNode* ASTNode::child(const chem::string_view &name) {
    switch(kind()) {
        case ASTNodeKind::VarInitStmt: {
            const auto stmt = as_var_init_unsafe();
            if (stmt->type) {
                return provide_child(stmt->type, name);
            } else if (stmt->value) {
                return provide_child(stmt->value->getType(), name);
            }
            return nullptr;
        }
        case ASTNodeKind::FunctionParam: {
            const auto param = as_func_param_unsafe();
            return provide_child(param->type, name);
        }
        case ASTNodeKind::GenericTypeParam: {
            const auto param = as_generic_type_param_unsafe();
            const auto linked = param->active_linked();
            return linked ? linked->child(name) : (param->at_least_type ? provide_child(param->at_least_type, name) : nullptr);
        }
        case ASTNodeKind::EnumDecl: {
            const auto decl = as_enum_decl_unsafe();
            auto mem = decl->members.find(name);
            if(mem == decl->members.end()) {
                const auto inherited = decl->get_inherited_enum_decl();
                return inherited ? inherited->child(name) : nullptr;
            } else {
                return mem->second;
            }
        }
        case ASTNodeKind::StructMember: {
            const auto mem = as_struct_member_unsafe();
            return provide_child(mem->type, name);
        }
        case ASTNodeKind::NamespaceDecl: {
            const auto ns = as_namespace_unsafe();
            auto node = ns->extended.find(name);
            if (node != ns->extended.end()) {
                return node->second;
            }
            return nullptr;
        }
        case ASTNodeKind::StructDecl: {
            const auto decl = as_struct_def_unsafe();
            auto node = ::child(decl, name);
            if (node) {
                return node;
            } else if (!decl->inherited.empty()) {
                for (auto& inherits : decl->inherited) {
                    if (inherits.specifier == AccessSpecifier::Public) {
                        const auto thing = provide_child(inherits.type, name);
                        if (thing) return thing;
                    }
                }
            };
            return nullptr;
        }
        case ASTNodeKind::CapturedVariable:
            return as_captured_var_unsafe()->linked->child(name);
        case ASTNodeKind::GenericStructDecl:
            return as_gen_struct_def_unsafe()->master_impl->ASTNode::child(name);
        case ASTNodeKind::GenericUnionDecl:
            return as_gen_union_decl_unsafe()->master_impl->ASTNode::child(name);
        case ASTNodeKind::GenericVariantDecl:
            return as_gen_variant_decl_unsafe()->master_impl->ASTNode::child(name);
        case ASTNodeKind::GenericInterfaceDecl:
            return as_gen_interface_decl_unsafe()->master_impl->ASTNode::child(name);
        case ASTNodeKind::GenericTypeDecl:
            return as_gen_type_decl_unsafe()->master_impl->child(name);
        case ASTNodeKind::GenericImplDecl:
            return as_gen_impl_decl_unsafe()->master_impl->ASTNode::child(name);
        case ASTNodeKind::EmbeddedNode:
            return as_embedded_node_unsafe()->child_res_fn(as_embedded_node_unsafe(), const_cast<chem::string_view*>(&name));
        case ASTNodeKind::TypealiasStmt: {
            return provide_child(as_typealias_unsafe()->actual_type, name);
        }
        case ASTNodeKind::ImportStmt:
            return ::child(as_import_stmt_unsafe(), name);
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::InterfaceDecl:
        case ASTNodeKind::UnionDecl:
            return ::child(as_members_container_unsafe(), name);
        case ASTNodeKind::VariantMemberParam: {
            return provide_child(as_variant_member_param_unsafe()->type, name);
        }
        case ASTNodeKind::StructType:
        case ASTNodeKind::UnionType:
        case ASTNodeKind::UnnamedUnion:
        case ASTNodeKind::UnnamedStruct:
            return ::child(as_variables_container(), name);
        case ASTNodeKind::VariantMember: {
            const auto mem = as_variant_member_unsafe();
            auto found = mem->values.find(name);
            if(found != mem->values.end()) {
                return (ASTNode*) &found->second;
            }
            return nullptr;
        }
        case ASTNodeKind::VariantCaseVariable: {
            const auto variant = as_variant_case_var_unsafe();
            if (variant->is_generic_param()) {
                const auto result = variant->member_param->child(name);
                return result;
            } else {
                return variant->member_param->child(name);
            }
        }
        case ASTNodeKind::PatternMatchId:
            return as_patt_match_id_unsafe()->member_param->child(name);
        default:
            return nullptr;
    }
}

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