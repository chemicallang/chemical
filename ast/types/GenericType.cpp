// Copyright (c) Chemical Language Foundation 2025.


#include "GenericType.h"
#include "LinkedType.h"
#include <algorithm>
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/utils/GenericUtils.h"

uint64_t GenericType::byte_size(const TargetData& target) {
    return referenced->byte_size(target);
}

bool GenericType::instantiate_inline(GenericInstantiatorAPI& instantiatorApi, SourceLocation loc) {

    const auto linked = referenced->linked;
    if(linked->kind() != ASTNodeKind::GenericTypeDecl) {
        return false;
    }

    // create the generic arguments
    const auto typeDecl = linked->as_gen_type_decl_unsafe();
    if (!typeDecl->is_partial_instantiate) {
        return false;
    }

    // get the diagnoser and allocator
    auto& diagnoser = instantiatorApi.getDiagnoser();
    auto& allocator = instantiatorApi.getAllocator();

    std::vector<TypeLoc> generic_args;

    // initialize the generic args
    const auto success = initialize_generic_args(diagnoser, generic_args, typeDecl->generic_params, types);
    if(!success) {
        return false;
    }

    // check all types have been inferred
    const auto success2 = check_inferred_generic_args(diagnoser, generic_args, typeDecl->generic_params, loc);
    if(!success2) {
        return false;
    }

    // create a copy
    const auto impl = typeDecl->copy_master(allocator);
    impl->attrs.is_inlined = true;

    // finalize signature using type decl
    GenericTypeDecl::finalize_signature(allocator, impl);

    // finalizes the signature
    instantiatorApi.FinalizeSignature(typeDecl, impl, generic_args);

    referenced->linked = impl;
    return true;
}

bool GenericType::instantiate(GenericInstantiatorAPI& instantiatorApi, SourceLocation loc, InstantiationRequirement requirement) {
    auto& diagnoser = instantiatorApi.getDiagnoser();
    const auto linked = referenced->linked;
    switch(linked->kind()) {
        case ASTNodeKind::GenericStructDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_struct_def_unsafe()->instantiate_type(instantiatorApi, types, loc, requirement);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericUnionDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_union_decl_unsafe()->instantiate_type(instantiatorApi, types, loc, requirement);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericInterfaceDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_interface_decl_unsafe()->instantiate_type(instantiatorApi, types, loc, requirement);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericVariantDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_variant_decl_unsafe()->instantiate_type(instantiatorApi, types, loc, requirement);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericTypeDecl: {
            // relink generic type decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_type_decl_unsafe()->instantiate_type(instantiatorApi, types, loc, requirement);
            if (!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericFuncDecl: {
            // a generic function referenced in a type position (like `apply<int>`)
            // instantiates the function and becomes the concrete declaration, which
            // itself carries the function type. Resolving it to the master
            // implementation instead would leak the declaration's own generic
            // parameters into the type (a self referencing function type)
            const auto gen_decl = linked->as_gen_func_decl_unsafe();
            std::vector<TypeLoc> generic_args;
            if(!initialize_generic_args(diagnoser, generic_args, gen_decl->generic_params, types)) {
                diagnoser.error("couldn't instantiate generic function type", loc);
                return false;
            }
            if(!check_inferred_generic_args(diagnoser, generic_args, gen_decl->generic_params, loc)) {
                diagnoser.error("couldn't instantiate generic function type", loc);
                return false;
            }
            // canonicalize the generic arguments (matches GenericFuncDecl::instantiate_call)
            for(auto& type : generic_args) {
                if(type) {
                    type = { type->canonical(), type.getLocation() };
                }
            }
            const auto concrete = gen_decl->register_generic_args(instantiatorApi, generic_args, loc, requirement);
            if(concrete == nullptr) {
                diagnoser.error("couldn't instantiate generic function type", loc);
                return false;
            }
            referenced->linked = concrete;
            break;
        }
        default:
            break;
    }
    return true;
}

GenericType* GenericType::copy(ASTAllocator& allocator) {
    auto gen = new (allocator.allocate<GenericType>()) GenericType((LinkedType*) referenced->copy(allocator));
    for(auto& type : types) {
        gen->types.emplace_back(type.copy(allocator));
    }
    return gen;
}

namespace {
// Two GenericType nodes may reference the same generic declaration through
// different AST nodes: the Generic*Decl wrapper, its master implementation, or a
// concrete instantiation. Normalize all of those to the Generic*Decl so
// `Foo<T>` compares equal regardless of which node it references.
ASTNode* canonical_generic_decl(ASTNode* n) {
    if(n == nullptr) {
        return nullptr;
    }
    switch(n->kind()) {
        case ASTNodeKind::GenericStructDecl:
        case ASTNodeKind::GenericUnionDecl:
        case ASTNodeKind::GenericVariantDecl:
        case ASTNodeKind::GenericInterfaceDecl:
        case ASTNodeKind::GenericTypeDecl:
        case ASTNodeKind::GenericFuncDecl:
            return n;
        default:
            break;
    }
    const auto container = n->get_members_container();
    if(container != nullptr && container->generic_parent != nullptr) {
        return (ASTNode*) container->generic_parent;
    }
    return n;
}
}

bool GenericType::is_same(BaseType *pure_type) {
    // Prefer the direct type: canonicalizing a GenericType can unwrap it to a
    // Linked form (BaseType::canonical, case Generic), which would make two
    // identical `Foo<T>` values compare unequal. Only fall back to canonical()
    // when the incoming type is a wrapper (literal / reference / alias).
    const auto other = pure_type->kind() == BaseTypeKind::Generic ? pure_type : pure_type->canonical();
    if(other->kind() == BaseTypeKind::Generic) {
        const auto other_gen = other->as_generic_type_unsafe();
        // same declaration: compare type args pairwise
        const auto this_decl = canonical_generic_decl(referenced->linked);
        const auto other_decl = canonical_generic_decl(other_gen->referenced->linked);
        if(this_decl && other_decl && this_decl == other_decl) {
            if (types.size() != other_gen->types.size()) return false;
            const auto min_size = std::min(types.size(), other_gen->types.size());
            for(unsigned i = 0; i < min_size; i++) {
                BaseType* this_arg = types[i];
                BaseType* other_arg = other_gen->types[i];
                if(!this_arg->is_same(other_arg)) {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
}

bool GenericType::satisfies(BaseType *pure_type) {
    const auto other = pure_type->canonical();
    if(other->kind() == BaseTypeKind::Generic) {
        const auto other_gen = other->as_generic_type_unsafe();
        // same declaration: compare type args pairwise
        const auto this_decl = canonical_generic_decl(referenced->linked);
        const auto other_decl = canonical_generic_decl(other_gen->referenced->linked);
        if(this_decl && other_decl && this_decl == other_decl) {
            const auto min_size = std::min(types.size(), other_gen->types.size());
            for(unsigned i = 0; i < min_size; i++) {
                BaseType* this_arg = types[i];
                BaseType* other_arg = other_gen->types[i];
                if(!this_arg->satisfies(other_arg)) {
                    return false;
                }
            }
            return true;
        }
    }
    // different declarations (inheritance) or non-generic other:
    // delegate to LinkedType::satisfies which handles inheritance, interfaces, etc.
    return referenced->satisfies(other);
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}