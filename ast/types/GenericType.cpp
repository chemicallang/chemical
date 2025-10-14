// Copyright (c) Chemical Language Foundation 2025.


#include "GenericType.h"
#include "LinkedType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/utils/GenericUtils.h"

uint64_t GenericType::byte_size(bool is64Bit) {
    return referenced->byte_size(is64Bit);
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
    const auto total = typeDecl->generic_params.size();
    std::vector<TypeLoc> generic_args(total, TypeLoc(nullptr));

    // default the generic args (to contain default type from generic parameters)
    default_generic_args(generic_args, typeDecl->generic_params, types);

    // check all types have been inferred
    unsigned i = 0;
    for(const auto arg : generic_args) {
        if(arg == nullptr) {
            diagnoser.error(arg.encoded_location()) << "couldn't infer type for generic parameter at index " << std::to_string(i);
            return false;
        }
        i++;
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
            const auto impl = linked->as_gen_struct_def_unsafe()->instantiate_type(instantiatorApi, types);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericUnionDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_union_decl_unsafe()->instantiate_type(instantiatorApi, types);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericInterfaceDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_interface_decl_unsafe()->instantiate_type(instantiatorApi, types);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericVariantDecl:{
            // relink generic struct decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_variant_decl_unsafe()->instantiate_type(instantiatorApi, types);
            if(!impl) {
                diagnoser.error("couldn't instantiate generic type", loc);
                return false;
            }
            referenced->linked = impl;
            break;
        }
        case ASTNodeKind::GenericTypeDecl: {
            // relink generic type decl with instantiated type, only if all types are specialized
            const auto impl = linked->as_gen_type_decl_unsafe()->instantiate_type(instantiatorApi, types);
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

bool GenericType::satisfies(BaseType *pure_type) {
    return referenced->satisfies(pure_type);
//    switch(pure_type->kind()) {
//        case BaseTypeKind::Generic:
//            return referenced->satisfies(pure_type->as_generic_type_unsafe()->referenced);
//        case BaseTypeKind::Linked:
//            return referenced->satisfies(pure_type);
//        default:
//            return false;
//    }
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}