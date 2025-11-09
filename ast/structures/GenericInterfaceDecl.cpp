// Copyright (c) Chemical Language Foundation 2025.

#include "GenericInterfaceDecl.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"
#include "std/except.h"

void GenericInterfaceDecl::finalize_signature(ASTAllocator& allocator, InterfaceDefinition* inst) {

    // copying the inherited types
    for(auto& inh : inst->inherited) {
        inh.type = inh.type.copy(allocator);
    }

    // copying the variables
    inst->copy_variables_in_place(allocator, inst);

    // finalizing the signature of functions
    for(const auto func : inst->master_functions()) {
        GenericFuncDecl::finalize_signature(allocator, func);
    }

}

void GenericInterfaceDecl::finalize_body(ASTAllocator& allocator, InterfaceDefinition* def) {

    for(const auto func : def->master_functions()) {
        GenericFuncDecl::finalize_body(allocator, func);
    }

}

InterfaceDefinition* GenericInterfaceDecl::register_generic_args(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& generic_args) {

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

    if(body_linked) {

        // signature and body both have been linked for master_impl
        // so all we need to do is
        finalize_signature(allocator, impl);
        finalize_body(allocator, impl);

        // now finalize using instantiator
        auto ptr = impl;
        const auto span = std::span<InterfaceDefinition*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);
        instantiator.FinalizeBody(this, span);

    } else if(signature_linked) {

        // copy over the extension functions, if more functions were linked because of link_signature called upon function declarations
        // TODO unsure this line is needed
        // impl->extension_functions = master_impl->extension_functions;

        // signature and body both have been linked for master_impl
        // so all we need to do is
        finalize_signature(allocator, impl);

        // now finalize using instantiator
        auto ptr = impl;
        const auto span = std::span<InterfaceDefinition*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);

    }

    return impl;

}

InterfaceDefinition* GenericInterfaceDecl::instantiate_type(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& types) {

    auto& diagnoser = instantiator.getDiagnoser();

    std::vector<TypeLoc> generic_args;

    // initialize the generic args
    const auto success = initialize_generic_args(diagnoser, generic_args, generic_params, types);
    if(!success) {
        return nullptr;
    }

    // check all types have been inferred
    const auto success2 = check_inferred_generic_args(diagnoser, generic_args, generic_params);
    if(!success2) {
        return nullptr;
    }

    return register_generic_args(instantiator, generic_args);


}

#ifdef COMPILER_BUILD

void GenericInterfaceDecl::code_gen_declare(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen_declare(gen);
        i++;
    }
}

void GenericInterfaceDecl::code_gen(Codegen &gen) {
    auto& i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen(gen);
        i++;
    }
}

void GenericInterfaceDecl::code_gen_external_declare(Codegen &gen) {
    // only declare the instantiations that have been bodied
    unsigned i = 0;
    const auto total = total_bodied_instantiations;
    while(i < total) {
        instantiations[i]->code_gen_external_declare(gen);
        i++;
    }
}

#endif