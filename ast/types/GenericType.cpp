// Copyright (c) Chemical Language Foundation 2025.


#include "GenericType.h"
#include "compiler/SymbolResolver.h"
#include "LinkedType.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/utils/GenericUtils.h"

bool GenericType::link(SymbolResolver &linker) {
    referenced->link(linker);
    const auto linked = referenced->linked;
    if(!linked) {
        return false;
    }
    for(auto& type : types) {
        if(!type->link(linker)) {
            return false;
        }
    }
    switch(linked->kind()) {
        case ASTNodeKind::GenericStructDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                referenced->linked = linked->as_gen_struct_def_unsafe()->register_generic_args(linker.genericInstantiator, types);
            }
            break;
        case ASTNodeKind::GenericUnionDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                referenced->linked = linked->as_gen_union_decl_unsafe()->register_generic_args(linker.genericInstantiator, types);
            }
            break;
        case ASTNodeKind::GenericInterfaceDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                referenced->linked = linked->as_gen_interface_decl_unsafe()->register_generic_args(linker.genericInstantiator, types);
            }
            break;
        case ASTNodeKind::GenericVariantDecl:
            if(are_all_specialized(types)) {
                // relink generic struct decl with instantiated type, only if all types are specialized
                referenced->linked = linked->as_gen_variant_decl_unsafe()->register_generic_args(linker.genericInstantiator, types);
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
        gen->types.emplace_back(type->copy(allocator));
    }
    return gen;
}

bool GenericType::satisfies(BaseType *pure_type) {
    if(pure_type->kind() == BaseTypeKind::Generic) {
        const auto gen_type = (GenericType*) pure_type;
        return referenced->is_same(gen_type->referenced);
    } else {
        return false;
    }
}

ASTNode *GenericType::linked_node() {
    return referenced->linked_node();
}