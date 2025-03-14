// Copyright (c) Chemical Language Foundation 2025.

#include "GenericUnionDecl.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"

void GenericUnionDecl::declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) {
    master_impl->take_members_from_parsed_nodes(linker);
    linker.declare(master_impl->name_view(), this);
}

void GenericUnionDecl::finalize_signature(ASTAllocator& allocator, UnionDef* inst) {

    // copying the inherited types
    for(auto& inh : inst->inherited) {
        inh.type = inh.type->copy(allocator);
    }

    // copying the variables
    inst->copy_variables_in_place(allocator, inst);

    // finalizing the signature of functions
    for(const auto func : inst->functions()) {
        GenericFuncDecl::finalize_signature(allocator, func);
    }

}

void GenericUnionDecl::finalize_body(ASTAllocator& allocator, UnionDef* def) {

    for(const auto func : def->functions()) {
        GenericFuncDecl::finalize_body(allocator, func);
    }

}

void GenericUnionDecl::link_signature(SymbolResolver &linker) {
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    master_impl->link_signature(linker);
    linker.scope_end();
    signature_linked = true;
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : instantiations) {
        finalize_signature(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(this, instantiations);
}

void GenericUnionDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    linker.scope_end();
    body_linked = true;
    // finalizing body of instantiations that occurred before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : instantiations) {
        finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(this, instantiations);
}

BaseType* GenericUnionDecl::create_value_type(ASTAllocator &allocator) {
    return master_impl->create_value_type(allocator);
}

UnionDef* GenericUnionDecl::register_generic_args(GenericInstantiatorAPI& instantiator, std::vector<BaseType*>& types) {

    const auto types_size = types.size();
    std::vector<BaseType*> generic_args(types_size);
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type;
        i++;
    }

    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();

    const auto itr = register_generic_usage(allocator, generic_params, generic_args);
    if(!itr.second) {
        // iteration already exist
        return instantiations[itr.first];
    }

    // we will do a complete instantiation right now
    const auto impl = master_impl->shallow_copy(allocator);

    if(itr.first != instantiations.size()) {
#ifdef DEBUG
        throw std::runtime_error("not the index we expected");
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
        const auto span = std::span<UnionDef*>(&ptr, 1);
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
        const auto span = std::span<UnionDef*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);

    }

    return impl;

}

#ifdef COMPILER_BUILD

void GenericUnionDecl::code_gen_declare(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen_declare(gen);
        i++;
    }
}

void GenericUnionDecl::code_gen(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen(gen);
        i++;
    }
    total_bodied_instantiations = instantiations.size();
}

void GenericUnionDecl::code_gen_external_declare(Codegen &gen) {
    // only declare the instantiations that have been bodied
    auto i = 0;
    while(i < total_bodied_instantiations) {
        instantiations[i]->code_gen_external_declare(gen);
        i++;
    }
}

#endif