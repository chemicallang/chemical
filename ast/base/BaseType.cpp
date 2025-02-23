// Copyright (c) Qinetik 2024.

#include "BaseType.h"
#include "preprocess/RepresentationVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/UnionDef.h"
#include "ast/types/LinkedType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/GenericType.h"
#include "ast/types/IntNType.h"
#include "ast/types/LiteralType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ArrayType.h"
#include <sstream>
#include "ASTNode.h"

std::string BaseType::representation() {
    std::ostringstream ostring;
    RepresentationVisitor visitor(ostring);
    visitor.visit(this);
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

bool BaseType::isStructLikeType() {
    switch(kind()) {
        case BaseTypeKind::Struct:
        case BaseTypeKind::Dynamic:
        case BaseTypeKind::Union:
            return true;
        case BaseTypeKind::Generic:
            return as_generic_type_unsafe()->referenced->isStructLikeType();
        case BaseTypeKind::Linked: {
            const auto linked = as_linked_type_unsafe()->linked;
            switch (linked->kind()) {
                case ASTNodeKind::VariantMember:
                case ASTNodeKind::StructDecl:
                case ASTNodeKind::UnnamedStruct:
                case ASTNodeKind::UnnamedUnion:
                case ASTNodeKind::VariantDecl:
                case ASTNodeKind::UnionDecl:
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

chem::string_view& BaseType::linked_name() {
    const auto k = kind();
    if(k == BaseTypeKind::Linked) {
        return ((LinkedType*) (this))->type;
    } else if(k == BaseTypeKind::Generic) {
        return ((GenericType*) (this))->referenced->type;
    } else {
#ifdef DEBUG
        throw std::runtime_error("BaseType::linked_name called on unexpected type '" + representation() + "'");
#else
        std::cerr << "BaseType::ref_name called on unexpected type '" + representation() << "'" << std::endl;
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
            default:
                return false;
        }
    }
}

bool BaseType::requires_move_fn() {
    auto node = get_direct_linked_node();
    if(!node) return false;
    auto node_kind = node->kind();
    if(ASTNode::isMembersContainer(node_kind)) {
        return ((MembersContainer*) node)->pre_move_func() != nullptr;
    } else {
        switch(node_kind) {
            case ASTNodeKind::VariantMember:
                return ((VariantMember*) node)->requires_move_fn();
            case ASTNodeKind::UnnamedStruct:
                return ((UnnamedStruct*) node)->requires_move_fn();
            default:
                return false;
        }
    }
}

bool BaseType::requires_clear_fn() {
    auto node = get_direct_linked_node();
    if(!node) return false;
    auto node_kind = node->kind();
    if(ASTNode::isMembersContainer(node_kind)) {
        return ((MembersContainer*) node)->clear_func() != nullptr;
    } else {
        switch(node_kind) {
            case ASTNodeKind::VariantMember:
                return ((VariantMember*) node)->requires_clear_fn();
            case ASTNodeKind::UnnamedStruct:
                return ((UnnamedStruct*) node)->requires_clear_fn();
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

BaseType* BaseType::pure_type() {
    switch(kind()) {
        case BaseTypeKind::Literal: {
            const auto type = as_literal_type_unsafe();
            return type->underlying;
        }
        case BaseTypeKind::Linked: {
            const auto linked = as_linked_type_unsafe()->linked;
            if (linked) {
                const auto known = linked->known_type();
                return known ? known : this;
            } else {
                return this;
            }
        }
        case BaseTypeKind::Pointer: {
            const auto ptr_type = as_pointer_type_unsafe();
            const auto pure_child = ptr_type->type->pure_type();
            if(pure_child && pure_child != ptr_type->type) {
                // TODO pointer type allocated without an allocator
                auto ptr = new PointerType(pure_child, ptr_type->encoded_location(), ptr_type->is_mutable);
                ptr_type->pures.emplace_back(ptr);
                return ptr;
            } else {
                return this;
            }
        }
        default:
            return this;
    }
}

BaseType* BaseType::pure_type(ASTAllocator& allocator) {
    switch(kind()) {
        case BaseTypeKind::Literal: {
            const auto type = as_literal_type_unsafe();
            return type->underlying;
        }
        case BaseTypeKind::Linked: {
            const auto linked = as_linked_type_unsafe()->linked;
            if (linked) {
                const auto known = linked->known_type();
                return known ? known : this;
            } else {
                return this;
            }
        }
        case BaseTypeKind::Pointer: {
            const auto ptr_type = as_pointer_type_unsafe();
            const auto pure_child = ptr_type->type->pure_type(allocator);
            if(pure_child && pure_child != ptr_type->type) {
                return new (allocator.allocate<PointerType>()) PointerType(pure_child, ptr_type->encoded_location(), ptr_type->is_mutable);
            } else {
                return this;
            }
        }
        case BaseTypeKind::Reference: {
            const auto ref_type = as_reference_type_unsafe();
            const auto pure_child = ref_type->type->pure_type(allocator);
            if(pure_child && pure_child != ref_type->type) {
                return new (allocator.allocate<ReferenceType>()) ReferenceType(pure_child, ref_type->encoded_location(), ref_type->is_mutable);
            } else {
                return this;
            }
        }
        default:
            return this;
    }
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
        default:
            return nullptr;
    }
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
        default:
            return nullptr;
    }
}

StructDefinition* BaseType::get_direct_linked_struct() {
    const auto ref_node = get_direct_linked_node();
    return ref_node ? ref_node->as_struct_def() : nullptr;
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

bool BaseType::requires_moving() {
    auto node = get_direct_linked_node();
    return node != nullptr && node->requires_moving(node->kind());
}

FunctionDeclaration* BaseType::implicit_constructor_for(ASTAllocator& allocator, Value *value) {
    const auto linked_def = linked_struct_def();
    if(linked_def) {
        const auto prev_itr = linked_def->active_iteration;
        const auto itr = get_generic_iteration();
        if(itr != -1) {
            linked_def->set_active_iteration(itr);
        }
        const auto implicit_constructor = linked_def->implicit_constructor_func(allocator, value);
        if(itr != -1) {
            linked_def->set_active_iteration(prev_itr);
        }
        return implicit_constructor;
    }
    return nullptr;
}

int16_t BaseType::set_generic_iteration(int16_t iteration) {
    if(iteration > -2) {
        const auto linked = linked_node();
        if(linked) {
            const auto members_container = linked->as_members_container();
            if (members_container) {
                return members_container->set_active_itr_ret_prev(iteration);
            }
        }
    }
    return -2;
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
    const auto pure = pure_type();
    if(pure->kind() == BaseTypeKind::Reference) {
        const auto ref = pure->as_reference_type_unsafe()->type->pure_type();
        const auto ref_kind = ref->kind();
        if(BaseType::isLoadableReferencee(ref_kind)) {
            return ref;
        }
    }
    return nullptr;
}

BaseType* BaseType::getAutoDerefType(BaseType* expected_type) {
    const auto value_type_pure = pure_type();
    const auto value_type_k = value_type_pure->kind();
    const auto exp_type_pure = expected_type->pure_type();
    const auto exp_type_k = exp_type_pure->kind();
    switch(value_type_k) {
        case BaseTypeKind::Reference: {
            const auto ref_type = value_type_pure->as_reference_type_unsafe();
            const auto pure_referencee = ref_type->type->pure_type();
            if((pure_referencee->is_same(exp_type_pure) || exp_type_pure->kind() == BaseTypeKind::Any) && BaseType::isIntNType(pure_referencee->kind())) {
                return pure_referencee;
            }
            return nullptr;
        }
        default:
            return nullptr;
    }
}

BaseType* BaseType::removeReferenceFromType(ASTAllocator& allocator) {
    const auto pure_t = pure_type(allocator);
    if(pure_t->kind() == BaseTypeKind::Reference) {
        return pure_t->as_reference_type_unsafe()->type;
    } else {
        return this;
    }
}

bool BaseType::satisfies(ASTAllocator& allocator, Value* value, bool assignment) {
    const auto val_type = value->create_type(allocator);
    return val_type != nullptr && satisfies(val_type->pure_type());
}

int16_t BaseType::get_generic_iteration() {
    switch(kind()) {
        case BaseTypeKind::Reference:
            return as_reference_type_unsafe()->type->get_generic_iteration();
        case BaseTypeKind::Pointer:
            return as_pointer_type_unsafe()->type->get_generic_iteration();
        case BaseTypeKind::Linked:{
            const auto container = as_linked_type_unsafe()->linked->as_members_container();
            return container ? container->active_iteration : (int16_t) -1;
        }
        case BaseTypeKind::Generic:
            return as_generic_type_unsafe()->generic_iteration;
        case BaseTypeKind::Array:
            return as_array_type_unsafe()->elem_type->get_generic_iteration();
        default:
            return -1;
    }
}

unsigned BaseType::type_alignment(bool is64Bit) {
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
        case BaseTypeKind::Reference:
        case BaseTypeKind::String:
        case BaseTypeKind::Void:
        case BaseTypeKind::Unknown:
        case BaseTypeKind::Dynamic:
            return 8;
        case BaseTypeKind::Bool:
            return 1;
        case BaseTypeKind::Double:
            break;
        case BaseTypeKind::Float:
            return 4;
        case BaseTypeKind::IntN:
            switch(as_intn_type_unsafe()->num_bits()) {
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
            return as_literal_type_unsafe()->underlying->type_alignment(is64Bit);
        case BaseTypeKind::Linked: {
            const auto pure = as_linked_type_unsafe()->pure_type();
            if(pure == this) {
                return 8;
            } else {
                return pure->type_alignment(is64Bit);
            }
        }
    }
}

BaseType::~BaseType() = default;