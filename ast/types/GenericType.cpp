// Copyright (c) Chemical Language Foundation 2025.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"
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

bool GenericType::link(SymbolResolver &linker, SourceLocation loc) {
    referenced->link(linker, loc);
    const auto linked = referenced->linked;
    if(!linked) {
        return false;
    }
    for(auto& type : types) {
        if(!type.link(linker)) {
            return false;
        }
    }
    switch(linked->kind()) {
        case ASTNodeKind::GenericStructDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                const auto impl = linked->as_gen_struct_def_unsafe()->instantiate_type(linker.genericInstantiator, types);
                if(!impl) {
                    linker.error("couldn't instantiate generic type", loc);
                    return false;
                }
                referenced->linked = impl;
            }
            break;
        case ASTNodeKind::GenericUnionDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                const auto impl = linked->as_gen_union_decl_unsafe()->instantiate_type(linker.genericInstantiator, types);
                if(!impl) {
                    linker.error("couldn't instantiate generic type", loc);
                    return false;
                }
                referenced->linked = impl;
            }
            break;
        case ASTNodeKind::GenericInterfaceDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                const auto impl = linked->as_gen_interface_decl_unsafe()->instantiate_type(linker.genericInstantiator, types);
                if(!impl) {
                    linker.error("couldn't instantiate generic type", loc);
                    return false;
                }
                referenced->linked = impl;
            }
            break;
        case ASTNodeKind::GenericVariantDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                const auto impl = linked->as_gen_variant_decl_unsafe()->instantiate_type(linker.genericInstantiator, types);
                if(!impl) {
                    linker.error("couldn't instantiate generic type", loc);
                    return false;
                }
                referenced->linked = impl;
            }
            break;
        case ASTNodeKind::GenericTypeDecl:
            if(are_all_specialized(types)) {
                // relink generic type decl with instantiated type, only if all types are specialized
                const auto impl = linked->as_gen_type_decl_unsafe()->instantiate_type(linker.genericInstantiator, types);
                if(!impl) {
                    linker.error("couldn't instantiate generic type", loc);
                    return false;
                }
                referenced->linked = impl;
            }
            break;
        default:
            break;
    }
    return true;
}

GenericType* GenericType::copy(ASTAllocator& allocator) const {
    auto gen = new (allocator.allocate<GenericType>()) GenericType((LinkedType*) referenced->copy(allocator));
    for(auto& type : types) {
        gen->types.emplace_back(type.copy(allocator));
    }
    return gen;
}

bool GenericType::satisfies(BaseType *pure_type) {
    switch(pure_type->kind()) {
        case BaseTypeKind::Generic:
            return referenced->satisfies(pure_type->as_generic_type_unsafe()->referenced);
        case BaseTypeKind::Linked:
            return referenced->satisfies(pure_type);
        default:
            return false;
    }
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}