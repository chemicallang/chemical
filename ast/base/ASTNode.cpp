// Copyright (c) Qinetik 2024.

#include "ASTNode.h"
#include "ASTUnit.h"
#include "BaseType.h"
#include "Value.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>

#if !defined(DEBUG) && defined(COMPILER_BUILD)
#include "compiler/Codegen.h"
#endif

std::string ASTNode::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    accept(&visitor);
    return ostring.str();
}

void ASTNode::runtime_name(std::ostream& stream) {
    const auto p = parent();
    if(p) p->runtime_name(stream);
    runtime_name_no_parent(stream);
}

void ASTNode::runtime_name_no_parent(std::ostream& stream) {
    stream << ns_node_identifier();
}

std::string ASTNode::runtime_name() {
    std::stringstream stream;
    runtime_name(stream);
    return stream.str();
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
            return as_struct_def_unsafe()->specifier;
        case ASTNodeKind::VariantDecl:
            return as_variant_def_unsafe()->specifier;
        case ASTNodeKind::NamespaceDecl:
            return as_namespace_unsafe()->specifier;
        case ASTNodeKind::UnionDecl:
            return as_union_def_unsafe()->specifier;
        case ASTNodeKind::EnumDecl:
            return as_enum_decl_unsafe()->specifier;
        case ASTNodeKind::FunctionDecl:
            return as_function_unsafe()->specifier;
        case ASTNodeKind::InterfaceDecl:
            return as_interface_def_unsafe()->specifier;
        default:
            return AccessSpecifier::Private;
    }
}

bool ASTNode::set_specifier(AccessSpecifier spec) {
    const auto k = kind();
    switch(k) {
        case ASTNodeKind::StructDecl:
            as_struct_def_unsafe()->specifier = spec;
            return true;
        case ASTNodeKind::VariantDecl:
            as_variant_def_unsafe()->specifier = spec;
            return true;
        case ASTNodeKind::NamespaceDecl:
            as_namespace_unsafe()->specifier = spec;
            return true;
        case ASTNodeKind::UnionDecl:
            as_union_def_unsafe()->specifier = spec;
            return true;
        case ASTNodeKind::EnumDecl:
            as_enum_decl_unsafe()->specifier = spec;
            return true;
        case ASTNodeKind::FunctionDecl:
            as_function_unsafe()->specifier = spec;
            return true;
        case ASTNodeKind::InterfaceDecl:
            as_interface_def_unsafe()->specifier = spec;
            return true;
        default:
            return false;
    }
}

bool ASTNode::is_exported() {
    return specifier() == AccessSpecifier::Public;
}

void ASTNode::set_parent(ASTNode* parent) {
#ifdef DEBUG
    throw std::runtime_error("set_parent called on base ast node");
#endif
}

#ifdef COMPILER_BUILD

llvm::Value *ASTNode::llvm_pointer(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_pointer called on bare ASTNode, with representation" + representation());
#else
    gen.early_error("ASTNode::llvm_pointer called, on node : " + representation());
    return nullptr;
#endif
};

llvm::Type *ASTNode::llvm_elem_type(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_elem_type called on bare ASTNode, with representation" + representation());
#else
    gen.early_error("ASTNode::llvm_elem_type called, on node : " + representation());
    return nullptr;
#endif
};

void ASTNode::code_gen(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("ASTNode code_gen called on bare ASTNode, with representation : " + representation());
#else
    gen.early_error("ASTNode::code_gen called, on node : " + representation());
#endif
}

bool ASTNode::add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) {
#ifdef DEBUG
    throw std::runtime_error("add_child_index called on a ASTNode");
#else
    gen.early_error("ASTNode::add_child_index called, on node : " + representation());
    return false;
#endif
}

llvm::Value *ASTNode::llvm_load(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_load called on a ASTNode");
#else
    gen.early_error("ASTNode::llvm_load called, on node : " + representation());
    return nullptr;
#endif
}

void ASTNode::code_gen_generic(Codegen &gen) {
#ifdef DEBUG
    throw std::runtime_error("llvm_load called on a ASTNode");
#else
    gen.early_error("ASTNode::llvm_load called, on node : " + representation());
#endif
}

#endif

void ASTNode::subscribe(GenericType* subscriber) {
#ifdef DEBUG
    throw std::runtime_error("ASTNode::subscribe called");
#else
    std::cerr << "ASTNode::subscibe called on node with representation " + representation() << std::endl;
#endif
}

int16_t ASTNode::get_active_iteration() {
    return -1;
}

void ASTNode::set_active_iteration(int16_t iteration) {
#ifdef DEBUG
    throw std::runtime_error("ASTNode::set_active_iteration called");
#else
    std::cerr << "ASTNode::set_active_iteration called on node with representation " + representation() << std::endl;
#endif
}

std::unique_ptr<BaseType> ASTNode::create_value_type() {
    return nullptr;
}

hybrid_ptr<BaseType> ASTNode::get_value_type() {
    return hybrid_ptr<BaseType> { nullptr, false };
}

ASTNode::~ASTNode() = default;

ASTUnit::ASTUnit() : scope(nullptr, nullptr) {}

ASTUnit::~ASTUnit() = default;