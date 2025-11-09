// Copyright (c) Chemical Language Foundation 2025.

#include "GenericTypeDecl.h"
#include "ast/structures/GenericTypeParameter.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/ASTDiagnoser.h"
#include "std/except.h"

void GenericTypeDecl::finalize_signature(ASTAllocator& allocator, TypealiasStatement* inst) {
    inst->actual_type = inst->actual_type.copy(allocator);
}

TypealiasStatement* GenericTypeDecl::copy_master(ASTAllocator& allocator) {
    // create a shallow copy
    const auto impl = master_impl->shallow_copy(allocator);
    impl->generic_parent = this;
    return impl;
}

TypealiasStatement* GenericTypeDecl::register_generic_args(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& generic_args) {

    auto& container = instantiator.getContainer();
    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();

    const auto itr = register_generic_usage(allocator, this, container, generic_args, ((std::vector<void*>&) instantiations));
    if(!itr.second) {
        // iteration already exists
        return instantiations[itr.first];
    }

    // we will do a complete instantiation right now
    const auto impl = master_impl->shallow_copy(allocator);

    if(itr.first != instantiations.size()) {
#ifdef DEBUG
        CHEM_THROW_RUNTIME("not the index we expected");
#endif
    }

    // store the pointer of instantiation
    instantiations.emplace_back(impl);

    // must set this variables, otherwise finalization won't be able to get which concrete implementation to use
    impl->generic_parent = this;
    impl->generic_instantiation = itr.first;

    if(signature_linked) {

        // signature and body both have been linked for master_impl
        // so all we need to do is
        finalize_signature(allocator, impl);

        // now finalize using instantiator
        auto ptr = impl;
        const auto span = std::span<TypealiasStatement*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);

    }

    return impl;

}

TypealiasStatement* GenericTypeDecl::instantiate_type(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& types, SourceLocation location) {

    auto& diagnoser = instantiator.getDiagnoser();

    std::vector<TypeLoc> generic_args;

    // initialize the generic args
    const auto success = initialize_generic_args(diagnoser, generic_args, generic_params, types);
    if(!success) {
        return nullptr;
    }

    // check all types have been inferred
    const auto success2 = check_inferred_generic_args(diagnoser, generic_args, generic_params, location);
    if(!success2) {
        return nullptr;
    }

    return register_generic_args(instantiator, generic_args);

}