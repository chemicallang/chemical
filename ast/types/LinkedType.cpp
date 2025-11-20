// Copyright (c) Chemical Language Foundation 2025.

#include "LinkedType.h"
#include "ast/statements/Typealias.h"
#include "StructType.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/types/ReferenceType.h"

uint64_t LinkedType::byte_size(TargetData& data) {
    return linked->byte_size(data);
}

bool is_struct_linked_satisfies(ASTNode* linked, ASTNode* other_linked, bool reference) {
    if(reference && !linked->as_struct_def_unsafe()->is_shallow_copyable()) {
        return false;
    }
    if (linked == other_linked) {
        return true;
    } else {
        const auto container = other_linked->get_members_container();
        return container && container->extends_node(linked);
    }
}

bool is_variant_linked_satisfies(ASTNode* linked, ASTNode* other_linked, bool reference) {
    if(reference & !linked->as_variant_def_unsafe()->is_shallow_copyable()) {
        return false;
    }
    if (linked == other_linked) {
        return true;
    } else {
        const auto other_kind = other_linked->kind();
        if(other_kind == ASTNodeKind::VariantMember) {
            return other_linked->as_variant_member_unsafe()->parent() == linked->as_variant_def_unsafe();
        } else {
            return false;
        }
    }
}

bool LinkedType::satisfies(BaseType *other_impure) {
    if(!linked) {
        return false;
    }
    const auto other = other_impure->canonical();
    const auto other_kind = other->kind();
    if(other_kind == BaseTypeKind::Linked && linked == ((LinkedType*) other)->linked) {
        return true;
    }
    switch(linked->kind()) {
        case ASTNodeKind::InterfaceDecl: {
            const auto other_linked = other->get_direct_linked_node();
            if(other_linked) {
                if (linked == other_linked) {
                    return true;
                } else {
                    const auto container = other_linked->get_members_container();
                    return container && container->extends_node(linked);
                }
            } else {
                break;
            }
        }
        case ASTNodeKind::StructDecl:{
            const auto other_linked = other->get_direct_linked_node();
            if(other_linked) {
                return is_struct_linked_satisfies(linked, other_linked, false);
            } else {
                if(other->kind() == BaseTypeKind::Reference) {
                    return is_struct_linked_satisfies(linked, other->as_reference_type_unsafe()->type->get_direct_linked_node(), true);
                } else {
                    break;
                }
            }
        }
        case ASTNodeKind::VariantDecl: {
            const auto other_linked = other->get_direct_linked_node();
            if(other_linked) {
                return is_variant_linked_satisfies(linked, other_linked, false);
            } else {
                if(other->kind() == BaseTypeKind::Reference) {
                    return is_variant_linked_satisfies(linked, other->as_reference_type_unsafe()->type->get_direct_linked_node(), true);
                } else {
                    break;
                }
            }
        }
        case ASTNodeKind::TypealiasStmt: {
            return linked->as_typealias_unsafe()->actual_type->satisfies(other);
        }
        case ASTNodeKind::EnumDecl: {
            return linked == other->get_direct_linked_node();
        }
        case ASTNodeKind::GenericTypeParam :{
            const auto param = linked->as_generic_type_param_unsafe();
            if(param->at_least_type) {
                return param->at_least_type->satisfies(other);
            } else {
                const auto known = param->known_type();
                if(known) {
                    return known->satisfies(other);
                } else {
                    // it's like a c++ template generic, which can take any type
                    return true;
                }
            }
        }
        default:
            break;
    }
    auto known = linked->known_type();
    if(known && known != this) {
        return known->satisfies(other);
    }
    return false;
}

bool LinkedType::satisfies(Value *value, bool assignment) {
    const auto valueType = value->getType();
    if(!valueType) return false;
    if(value->isValueIntegerLiteral()) {
        if(!linked) return false;
        const auto known = linked->known_type();
        if (known) {
            const auto canonical = known->canonical();
            if(canonical->kind() == BaseTypeKind::IntN) {
                return true;
            }
        }
    }
    return satisfies(valueType);
}