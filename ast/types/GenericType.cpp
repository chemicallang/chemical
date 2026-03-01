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
#include "ast/utils/GenericUtils.h"

uint64_t GenericType::byte_size(TargetData& target) {
    return referenced->byte_size(target);
}

bool GenericType::instantiate_inline(GenericInstantiatorAPI& instantiatorApi, SourceLocation loc) {
    auto& diagnoser = instantiatorApi.getDiagnoser();
    const auto linked = referenced->linked;
    if(linked->kind() != ASTNodeKind::GenericTypeDecl) {
        return true;
    }

    auto& allocator = instantiatorApi.getAllocator();

    // create the generic arguments
    const auto typeDecl = linked->as_gen_type_decl_unsafe();
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

     if(typeDecl->signature_linked) {

        // finalize signature using type decl
        GenericTypeDecl::finalize_signature(allocator, impl);

        // finalizes the signature
        instantiatorApi.FinalizeSignature(typeDecl, impl, generic_args);

     } else {

         typeDecl->inline_instantiations.emplace_back(impl, std::move(generic_args));

     }

    referenced->linked = impl;
    return true;
}

bool GenericType::instantiate(GenericInstantiatorAPI& instantiatorApi, SourceLocation loc) {
    auto& diagnoser = instantiatorApi.getDiagnoser();
    const auto linked = referenced->linked;
    switch(linked->kind()) {
        case ASTNodeKind::GenericStructDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_struct_def_unsafe()->instantiate_type(instantiatorApi, types, loc);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericUnionDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_union_decl_unsafe()->instantiate_type(instantiatorApi, types, loc);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericInterfaceDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_interface_decl_unsafe()->instantiate_type(instantiatorApi, types, loc);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericVariantDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_variant_decl_unsafe()->instantiate_type(instantiatorApi, types, loc);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericTypeDecl: {
            // relink generic type decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_type_decl_unsafe()->instantiate_type(instantiatorApi, types, loc);
            if (!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
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

bool GenericType::is_same(BaseType *pure_type) {
    const auto other = pure_type->canonical();
    if(other->kind() == BaseTypeKind::Generic) {
        const auto other_gen = other->as_generic_type_unsafe();
        // same declaration: compare type args pairwise
        if(referenced->linked && other_gen->referenced->linked && referenced->linked == other_gen->referenced->linked) {
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
        if(referenced->linked && other_gen->referenced->linked && referenced->linked == other_gen->referenced->linked) {
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