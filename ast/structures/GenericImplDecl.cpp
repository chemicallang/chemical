// Copyright (c) Chemical Language Foundation 2025.

#include "GenericImplDecl.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"
#include "std/except.h"

void GenericImplDecl::finalize_signature(ASTAllocator& allocator, ImplDefinition* inst) {

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

void GenericImplDecl::finalize_body(ASTAllocator& allocator, ImplDefinition* def) {

    for(const auto func : def->master_functions()) {
        GenericFuncDecl::finalize_body(allocator, func);
    }

}

ImplDefinition* GenericImplDecl::register_generic_args(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& types) {

    const auto types_size = types.size();
    std::vector<TypeLoc> generic_args(types_size, TypeLoc(nullptr));
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type;
        i++;
    }

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
        const auto span = std::span<ImplDefinition*>(&ptr, 1);
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
        const auto span = std::span<ImplDefinition*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);

    }

    return impl;

}

ImplDefinition* GenericImplDecl::instantiate_type(GenericInstantiatorAPI& instantiator, std::vector<TypeLoc>& types) {
    auto& diagnoser = instantiator.getDiagnoser();

    std::vector<TypeLoc> generic_args;

    // default the generic args (to contain default type from generic parameters)
    const auto success = default_generic_args(diagnoser, generic_args, generic_params, types);
    if(!success) {
        return nullptr;
    }

    // check all types have been inferred
    unsigned i = 0;
    for(const auto arg : generic_args) {
        if(arg == nullptr) {
            diagnoser.error(arg.encoded_location()) << "couldn't infer type for generic parameter at index " << std::to_string(i);
            return nullptr;
        }
        i++;
    }

    return register_generic_args(instantiator, generic_args);
}

#ifdef COMPILER_BUILD

void GenericImplDecl::code_gen_declare(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen_declare(gen);
        i++;
    }
}

void GenericImplDecl::code_gen(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen(gen);
        i++;
    }
    total_bodied_instantiations = instantiations.size();
}

void GenericImplDecl::code_gen_external_declare(Codegen &gen) {
    // only declare the instantiations that have been bodied
    auto i = 0;
    while(i < total_bodied_instantiations) {
        instantiations[i]->code_gen_external_declare(gen);
        i++;
    }
}

#endif