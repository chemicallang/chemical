// Copyright (c) Chemical Language Foundation 2025.

#include "BaseType.h"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/UnionDef.h"
#include "ast/types/LinkedType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/CapturingFunctionType.h"
#include "ast/types/GenericType.h"
#include "ast/types/IntNType.h"
#include "ast/types/LiteralType.h"
#include "ast/types/MaybeRuntimeType.h"
#include "ast/types/RuntimeType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ArrayType.h"
#include <sstream>
#include <iostream>
#include "ASTNode.h"
#include "std/except.h"

std::string BaseType::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    visitor.visit(this);
    return ostring.str();
}

bool BaseType::isStructLikeType() {
    switch(kind()) {
        case BaseTypeKind::Struct:
        case BaseTypeKind::Dynamic:
        case BaseTypeKind::Union:
            return true;
        case BaseTypeKind::Generic:
            return as_generic_type_unsafe()->referenced->isStructLikeType();
        case BaseTypeKind::CapturingFunction:
            return as_capturing_func_type_unsafe()->instance_type->isStructLikeType();
        case BaseTypeKind::Linked: {
            const auto linked = as_linked_type_unsafe()->linked;
            switch (linked->kind()) {
                case ASTNodeKind::VariantMember:
                case ASTNodeKind::StructDecl:
                case ASTNodeKind::UnnamedStruct:
                case ASTNodeKind::UnnamedUnion:
                case ASTNodeKind::VariantDecl:
                case ASTNodeKind::UnionDecl:
                case ASTNodeKind::InterfaceDecl:
                case ASTNodeKind::GenericStructDecl:
                case ASTNodeKind::GenericVariantDecl:
                case ASTNodeKind::GenericUnionDecl:
                case ASTNodeKind::GenericInterfaceDecl:
                    return true;
                case ASTNodeKind::TypealiasStmt:
                    return linked->as_typealias_unsafe()->actual_type->isStructLikeType();
                case ASTNodeKind::GenericTypeParam: {
                    const auto known = linked->as_generic_type_param_unsafe()->known_type();
                    if(known) {
                        return known->isStructLikeType();
                    } else {
                        return false;
                    }
                }
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

chem::string_view BaseType::linked_name() {
    const auto k = kind();
    if(k == BaseTypeKind::Linked) {
        return ((LinkedType*) (this))->linked_name();
    } else if(k == BaseTypeKind::Generic) {
        return ((GenericType*) (this))->referenced->linked_name();
    } else {
#ifdef DEBUG
        CHEM_THROW_RUNTIME("BaseType::linked_name called on unexpected type'");
#else
        std::cerr << "BaseType::linked_name called on unexpected type '" + representation() << "'" << std::endl;
        chem::string_view view("");
        return view;
#endif
    }
}

MembersContainer* BaseType::get_members_container() {
    const auto direct_node = get_direct_linked_node();
    return direct_node ? direct_node->get_members_container() : nullptr;
}

FunctionDeclaration* BaseType::get_def_constructor() {
    auto container = get_members_container();
    return container ? container->default_constructor_func() : nullptr;
}

FunctionDeclaration* BaseType::get_destructor() {
    auto container = get_members_container();
    return container ? container->destructor_func() : nullptr;
}

FunctionDeclaration* BaseType::get_copy_fn() {
    auto container = get_members_container();
    return container ? container->copy_func() : nullptr;
}

bool BaseType::requires_destructor() {
    if(kind() == BaseTypeKind::CapturingFunction) {
        return as_capturing_func_type_unsafe()->instance_type->requires_destructor();
    }
    auto node = get_direct_linked_node();
    if(!node) return false;
    auto node_kind = node->kind();
    if(ASTNode::isMembersContainer(node_kind)) {
        return ((MembersContainer*) node)->destructor_func() != nullptr;
    } else {
        switch(node_kind) {
            case ASTNodeKind::VariantMember:
                return ((VariantMember*) node)->requires_destructor();
            case ASTNodeKind::UnnamedStruct:
                return ((UnnamedStruct*) node)->requires_destructor();
            case ASTNodeKind::TypealiasStmt:
                return ((TypealiasStatement*) node)->actual_type->requires_destructor();
            default:
                return false;
        }
    }
}

bool BaseType::requires_copy_fn() {
    auto node = get_direct_linked_node();
    if(!node) return false;
    auto node_kind = node->kind();
    if(ASTNode::isMembersContainer(node_kind)) {
        return ((MembersContainer*) node)->copy_func() != nullptr;
    } else {
        switch(node_kind) {
            case ASTNodeKind::VariantMember:
                return ((VariantMember*) node)->requires_copy_fn();
            case ASTNodeKind::UnnamedStruct:
                return ((UnnamedStruct*) node)->requires_copy_fn();
            default:
                return false;
        }
    }
}

BaseType* BaseType::canonical() {
    switch(kind()) {
        case BaseTypeKind::Literal:
            return as_literal_type_unsafe()->underlying;
        case BaseTypeKind::MaybeRuntime:
            return as_maybe_runtime_type_unsafe()->underlying;
        case BaseTypeKind::Runtime:
            return as_runtime_type_unsafe()->underlying;
        case BaseTypeKind::Linked: {
            const auto linked = as_linked_type_unsafe()->linked;
            if (linked) {
                const auto known = linked->known_type();
                return known ? known != this ? known->canonical() : known : this;
            } else {
                return this;
            }
        }
        case BaseTypeKind::Generic: {
            const auto gen = as_generic_type_unsafe();
            return gen->referenced->canonical();
        }
        default:
            return this;
    }
}

BaseType* BaseType::canonicalize_enum() {
    const auto decl = get_direct_linked_enum();
    return decl ? decl->get_underlying_integer_type() : this;
}

ASTNode* BaseType::get_direct_linked_node() {
    switch(kind()) {
        case BaseTypeKind::Linked:
            return ((LinkedType*) this)->linked;
        case BaseTypeKind::Generic:
            return ((GenericType*) this)->referenced->linked;
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union:
            return linked_node();
        case BaseTypeKind::CapturingFunction:
            return as_capturing_func_type_unsafe()->instance_type->get_direct_linked_node();
        default:
            return nullptr;
    }
}

ASTNode* BaseType::get_direct_linked_canonical_node() {
    const auto n = get_direct_linked_node();
    if(n && n->kind() == ASTNodeKind::TypealiasStmt) {
        return n->as_typealias_unsafe()->actual_type->get_direct_linked_canonical_node();
    } else {
        return n;
    };
}

ASTNode* BaseType::get_linked_node(bool include_ref, bool include_ptr) {
    switch(kind()) {
        case BaseTypeKind::Linked:
            return as_linked_type_unsafe()->linked;
        case BaseTypeKind::Generic:
            return as_generic_type_unsafe()->referenced->linked;
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union:
            return linked_node();
        case BaseTypeKind::Pointer:
            if(include_ptr) {
                return as_pointer_type_unsafe()->type->get_linked_node(false, false);
            } else {
                return nullptr;
            }
        case BaseTypeKind::Reference:
            if(include_ref) {
                return as_reference_type_unsafe()->type->get_linked_node(false, false);
            } else {
                return nullptr;
            }
        case BaseTypeKind::CapturingFunction:
            return as_capturing_func_type_unsafe()->instance_type->get_direct_linked_node();
        default:
            return nullptr;
    }
}

ASTNode* BaseType::get_linked_canonical_node(bool include_ref, bool include_ptr) {
    const auto n = get_linked_node(include_ref, include_ptr);
    if(n && n->kind() == ASTNodeKind::TypealiasStmt) {
        return n->as_typealias_unsafe()->actual_type->get_linked_canonical_node(include_ref, include_ptr);
    } else {
        return n;
    };
}

ASTNode* BaseType::get_ref_or_linked_node() {
    switch(kind()) {
        case BaseTypeKind::Linked:
            return ((LinkedType*) this)->linked;
        case BaseTypeKind::Generic:
            return ((GenericType*) this)->referenced->linked;
        case BaseTypeKind::Reference:
            return ((ReferenceType*) this)->type->get_direct_linked_node();
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union:
            return linked_node();
        case BaseTypeKind::CapturingFunction:
            return as_capturing_func_type_unsafe()->instance_type->get_direct_linked_node();
        default:
            return nullptr;
    }
}

StructDefinition* BaseType::get_direct_linked_struct() {
    const auto ref_node = get_direct_linked_node();
    return ref_node ? ref_node->as_struct_def() : nullptr;
}

MembersContainer* BaseType::get_direct_linked_container() {
    const auto ref_node = get_direct_linked_canonical_node();
    return ref_node ? ref_node->as_members_container() : nullptr;
}

InterfaceDefinition* BaseType::get_direct_linked_interface() {
    const auto ref_node = get_direct_linked_node();
    return ref_node ? ref_node->as_interface_def() : nullptr;
}

StructDefinition* BaseType::get_ref_or_linked_struct() {
    const auto node = get_ref_or_linked_node();
    return node ? node->as_struct_def() : nullptr;
}

VariantDefinition* BaseType::get_direct_linked_variant() {
    const auto ref_node = get_direct_linked_node();
    return ref_node ? ref_node->as_variant_def() : nullptr;
}

EnumDeclaration* BaseType::get_direct_linked_enum() {
    const auto ref_node = get_direct_linked_node();
    return ref_node ? ref_node->as_enum_decl() : nullptr;
}

StructDefinition* BaseType::get_direct_linked_movable_struct() {
    const auto direct_ref_struct = get_direct_linked_struct();
    if(direct_ref_struct && (direct_ref_struct->is_shallow_copyable() || direct_ref_struct->destructor_func())) {
        return direct_ref_struct;
    } else {
        return nullptr;
    }
}

StructDefinition* BaseType::get_direct_non_movable_struct() {
    const auto direct_ref_struct = get_direct_linked_struct();
    if(direct_ref_struct && !direct_ref_struct->is_shallow_copyable() && direct_ref_struct->destructor_func() == nullptr) {
        return direct_ref_struct;
    } else {
        return nullptr;
    }
}

inline FunctionType* get_func_type_from_linked(LinkedType* type) {
    const auto l = type->linked;
    if(l->kind() == ASTNodeKind::TypealiasStmt) {
        return l->as_typealias_unsafe()->actual_type->get_canonical_function_type();
    } else {
        return nullptr;
    }
}

FunctionType* BaseType::get_canonical_function_type() {
    switch(kind()) {
        case BaseTypeKind::Function:
            return (FunctionType*) this;
        case BaseTypeKind::Linked:
            return get_func_type_from_linked(as_linked_type_unsafe());
        case BaseTypeKind::Generic:
            return get_func_type_from_linked(as_generic_type_unsafe()->referenced);
        case BaseTypeKind::CapturingFunction:
            return as_capturing_func_type_unsafe()->func_type->get_canonical_function_type();
        default:
            return nullptr;
    }
}

FunctionType* BaseType::get_function_type() {
    switch(kind()) {
        case BaseTypeKind::Function:
            return (FunctionType*) this;
        case BaseTypeKind::CapturingFunction:
            return as_capturing_func_type_unsafe()->func_type->as_function_type();
        default:
            return nullptr;
    }
}

bool BaseType::requires_moving() {
    auto node = get_direct_linked_node();
    return node != nullptr && node->requires_moving(node->kind());
}

FunctionDeclaration* BaseType::implicit_constructor_for(Value *value) {
    const auto linked = linked_node();
    if(linked == nullptr || linked->kind() != ASTNodeKind::StructDecl) return nullptr;
    const auto linked_def = linked->as_struct_def_unsafe();
    if(linked_def) {
        const auto implicit_constructor = linked_def->implicit_constructor_func(value);
        return implicit_constructor;
    }
    return nullptr;
}

bool BaseType::isIntegerLikeStorage() {
    switch(kind()) {
        case BaseTypeKind::IntN:
            return true;
        case BaseTypeKind::Linked:{
            const auto linked = as_linked_type_unsafe()->linked;
            switch(linked->kind()) {
                case ASTNodeKind::EnumDecl:
                    return true;
                case ASTNodeKind::TypealiasStmt: {
                    return linked->as_typealias_unsafe()->actual_type->isIntegerLikeStorage();
                }
                default:
                    return false;
            }
        }
        default:
            return false;
    }
}

bool BaseType::isCharType() {
    if(kind() != BaseTypeKind::IntN) return false;
    return as_intn_type_unsafe()->IntNKind() == IntNTypeKind::Char;
}

bool BaseType::isStringType() {
    switch(kind()) {
        case BaseTypeKind::String:
            return true;
        case BaseTypeKind::Pointer:
            return as_pointer_type_unsafe()->type->isCharType();
        default:
            return false;
    }
}

bool BaseType::make_mutable() {
    switch(kind()) {
        case BaseTypeKind::Pointer: {
            ((PointerType*) this)->is_mutable = true;
            const auto ref = ((PointerType*) this)->type;
            return ref->make_mutable();
        }
        case BaseTypeKind::Reference: {
            ((ReferenceType*) this)->is_mutable = true;
            const auto ref = ((ReferenceType*) this)->type;
            return ref->make_mutable();
        }
        default:
            // this method is called before symbol resolution, linked types are not linked
            // linked types usually ref a struct / union / variant / interface which are mutable
            // unless linked types ref a typealias, whose internal type we cannot make mutable
            // all other int n types are determined to be mutable on the basis of 'var' keyword in var init
            // or in struct member, basically their variable's value is changeable
            // mutable means change that can be done within a struct is possible
            // and var means directly changing a storage location to hold a different value is possible
            return true;
    }
}

bool BaseType::is_mutable() {
    switch(kind()) {
        case BaseTypeKind::Generic:
        case BaseTypeKind::Dynamic:
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union: {
            // direct struct / union / variant / interface are all mutable
            return true;
        }
        case BaseTypeKind::Array: {
            // array types are mutable
            return true;
        }
        case BaseTypeKind::Linked: {
            const auto linked = ((LinkedType*) this)->linked;
            const auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::TypealiasStmt) {
                const auto actual = linked->as_typealias_unsafe()->actual_type;
                return actual->is_mutable();
            } else {
                // direct struct / union / variant / interface are all mutable
                return true;
            }
        }
        case BaseTypeKind::Pointer:
            return ((PointerType*) this)->is_mutable;
        case BaseTypeKind::Reference:
            return ((ReferenceType*) this)->is_mutable;
        default:
            // int n types aren't mutable, their mutability is checked based on 'var' keyword
            // 'var' means variable can be changed, 'mut' means variable can be changed within (members of struct)
            return false;
    }
}

inline bool isNodeReferenceCanonical(ASTNode* node) {
    return node->kind() == ASTNodeKind::TypealiasStmt && node->as_typealias_unsafe()->actual_type->isReferenceCanonical();
}

bool BaseType::isReferenceCanonical() {
    switch(kind()) {
        case BaseTypeKind::Reference:
            return true;
        case BaseTypeKind::Linked:
            return isNodeReferenceCanonical(as_linked_type_unsafe()->linked);
        case BaseTypeKind::Generic:
            return isNodeReferenceCanonical(as_generic_type_unsafe()->referenced->linked);
        default:
            return false;
    }
}

bool BaseType::is_reference_to(ASTNode* node) {
    if(kind() == BaseTypeKind::Reference) {
        const auto child_type = ((ReferenceType*) this)->type;
        const auto child_type_kind = child_type->kind();
        if(child_type_kind == BaseTypeKind::Linked && ((LinkedType*) child_type)->linked == node) {
            return true;
        }
    }
    return false;
}

BaseType* BaseType::getLoadableReferredType() {
    if(kind() == BaseTypeKind::Reference) {
        const auto ref = as_reference_type_unsafe()->type->canonical();
        const auto ref_kind = ref->kind();
        if(BaseType::isLoadableReferencee(ref_kind)) {
            return ref;
        }
    }
    return nullptr;
}

BaseType* BaseType::getAutoDerefType(BaseType* expected_type) {
    const auto value_type_pure = canonical();
    const auto value_type_k = value_type_pure->kind();
    const auto exp_type_pure = expected_type->canonical();
    const auto exp_type_k = exp_type_pure->kind();
    switch(value_type_k) {
        case BaseTypeKind::Reference: {
            const auto ref_type = value_type_pure->as_reference_type_unsafe();
            const auto pure_referencee = ref_type->type->canonical();
            if((pure_referencee->is_same(exp_type_pure) || exp_type_pure->kind() == BaseTypeKind::Any) && BaseType::isIntNType(pure_referencee->kind())) {
                return pure_referencee;
            }
            return nullptr;
        }
        default:
            return nullptr;
    }
}

BaseType* BaseType::removeReferenceFromType() {
    const auto pure_t = canonical();
    if(pure_t->kind() == BaseTypeKind::Reference) {
        return pure_t->as_reference_type_unsafe()->type;
    } else {
        return this;
    }
}

bool BaseType::satisfies(Value* value, bool assignment) {
    const auto val_type = value->getType();
    return satisfies(val_type->canonical());
}

unsigned BaseType::type_alignment(TargetData& data) {
    switch(kind()) {
        case BaseTypeKind::Any:
        case BaseTypeKind::Array:
        case BaseTypeKind::Struct:
        case BaseTypeKind::Union:
        case BaseTypeKind::LongDouble:
        case BaseTypeKind::Complex:
        case BaseTypeKind::Float128:
        case BaseTypeKind::Function:
        case BaseTypeKind::Generic:
        case BaseTypeKind::Pointer:
        case BaseTypeKind::NullPtr:
        case BaseTypeKind::Reference:
        case BaseTypeKind::String:
        case BaseTypeKind::Void:
        case BaseTypeKind::Unknown:
        case BaseTypeKind::Dynamic:
            return 8;
        case BaseTypeKind::Bool:
            return 1;
        case BaseTypeKind::Double:
            return 8;
        case BaseTypeKind::Float:
            return 4;
        case BaseTypeKind::IntN:
            switch(as_intn_type_unsafe()->num_bits(data)) {
                case 8:
                    return 1;
                case 16:
                    return 2;
                case 32:
                    return 4;
                case 64:
                default:
                    return 8;
            }
        case BaseTypeKind::Literal:
            return as_literal_type_unsafe()->underlying->type_alignment(data);
        case BaseTypeKind::Linked: {
            const auto pure = as_linked_type_unsafe()->canonical();
            if(pure == this) {
                return 8;
            } else {
                return pure->type_alignment(data);
            }
        }
        case BaseTypeKind::ExpressionType:
            // TODO
            return 8;
        default:
            return 0;
    }
}

uint64_t BaseType::byte_size(TargetData& data) {
    CHEM_THROW_RUNTIME("byte_size called on base type");
    return 0;
}

BaseType::~BaseType() = default;