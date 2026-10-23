// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <unordered_set>

#include "ast/base/BaseType.h"
#include "ast/base/BaseTypeKind.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/StructMember.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/types/ArrayType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/GenericType.h"
#include "ast/types/LinkedType.h"
#include "compiler/symres/ImplementationsIndex.h"

/**
 * Structural derivation for the `Send` and `Sync` auto traits.
 *
 * This is deliberately conservative: a type is Send/Sync only when every
 * field is. Raw pointers are neither (moving one to another thread is not
 * automatically sound), references follow the usual rules, and generic
 * arguments participate as well (so `PhantomData<*mut T>` is not Send).
 *
 * Explicit negative impls (`impl !Send for T`) are not modelled yet; callers
 * that need an exception must wait for the negative-impl syntax. A false
 * positive is avoided by requiring the whole field graph to qualify.
 */
enum class AutoTraitKind {
    Send,
    Sync,
};

inline bool type_satisfies_auto_trait(
    BaseType* type,
    AutoTraitKind kind,
    std::unordered_set<MembersContainer*>& visiting,
    ImplementationsIndex* index = nullptr,
    ASTNode* send_interface = nullptr,
    ASTNode* sync_interface = nullptr
);

inline bool auto_trait_members(
    MembersContainer* container,
    AutoTraitKind kind,
    std::unordered_set<MembersContainer*>& visiting,
    ImplementationsIndex* index = nullptr,
    ASTNode* send_interface = nullptr,
    ASTNode* sync_interface = nullptr
) {
    if(container == nullptr) return true;
    if(index != nullptr) {
        ASTNode* iface = kind == AutoTraitKind::Send ? send_interface : sync_interface;
        if(iface != nullptr) {
            if(index->has_negative_impl(iface, (ASTAny*) container)) return false;
            if(index->get_impl(iface, (ASTAny*) container) != nullptr) return true;
            if(container->generic_parent != nullptr &&
               index->has_unconditional_generic_negative_impl(
                   (ASTNode*) container->generic_parent, iface)) {
                return false;
            }
        }
    }
    if(!visiting.insert(container).second) {
        // A cycle can only be reached through a pointer/reference, both of
        // which have already been handled; treat the repeated node as fine.
        return true;
    }
    for(auto* member : container->variables()) {
        switch(member->kind()) {
            case ASTNodeKind::StructMember: {
                auto* field = (StructMember*) member;
                if(!type_satisfies_auto_trait(
                    const_cast<BaseType*>(field->type.getType()), kind, visiting,
                    index, send_interface, sync_interface
                )) {
                    visiting.erase(container);
                    return false;
                }
                break;
            }
            case ASTNodeKind::VariantMember: {
                auto* variant = (VariantMember*) member;
                for(auto& entry : variant->values) {
                    auto* param = entry.second;
                    if(param != nullptr && !type_satisfies_auto_trait(
                        const_cast<BaseType*>(param->type.getType()), kind, visiting,
                        index, send_interface, sync_interface
                    )) {
                        visiting.erase(container);
                        return false;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    visiting.erase(container);
    return true;
}

inline bool type_satisfies_auto_trait(
    BaseType* type,
    AutoTraitKind kind,
    std::unordered_set<MembersContainer*>& visiting,
    ImplementationsIndex* index,
    ASTNode* send_interface,
    ASTNode* sync_interface
) {
    if(type == nullptr) return true;
    switch(type->kind()) {
        case BaseTypeKind::Array:
            return type_satisfies_auto_trait(
                const_cast<BaseType*>(type->as_array_type_unsafe()->elem_type.getType()),
                kind, visiting, index, send_interface, sync_interface
            );
        case BaseTypeKind::Pointer:
            // *T and *mut T are neither Send nor Sync.
            return false;
        case BaseTypeKind::Reference: {
            auto* ref = type->as_reference_type_unsafe();
            auto* inner = ref->type;
            if(kind == AutoTraitKind::Sync) {
                // &T and &mut T are Sync when T is Sync.
                return type_satisfies_auto_trait(inner, AutoTraitKind::Sync, visiting,
                                                 index, send_interface, sync_interface);
            }
            // &T is Send when T is Sync; &mut T is Send when T is Send.
            return type_satisfies_auto_trait(
                inner, ref->is_mutable ? AutoTraitKind::Send : AutoTraitKind::Sync, visiting,
                index, send_interface, sync_interface
            );
        }
        case BaseTypeKind::Function:
            return true;
        case BaseTypeKind::CapturingFunction:
            // The captured environment is not modelled; be conservative.
            return false;
        case BaseTypeKind::Generic: {
            auto* gen = type->as_generic_type_unsafe();
            for(auto& arg : gen->types) {
                if(!type_satisfies_auto_trait(
                    const_cast<BaseType*>(arg.getType()), kind, visiting,
                    index, send_interface, sync_interface
                )) {
                    return false;
                }
            }
            return auto_trait_members(type->get_members_container(), kind, visiting,
                                      index, send_interface, sync_interface);
        }
        default:
            break;
    }
    auto* linked = type->get_direct_linked_node();
    if(linked != nullptr && linked->kind() == ASTNodeKind::GenericTypeParam) {
        // A bare `T` is not automatically Send/Sync without a bound.
        return false;
    }
    auto* container = type->get_members_container();
    if(container == nullptr) {
        // primitives and other non-container types
        return true;
    }
    return auto_trait_members(container, kind, visiting,
                              index, send_interface, sync_interface);
}
