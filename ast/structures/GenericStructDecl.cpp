// Copyright (c) Chemical Language Foundation 2025.

#include "GenericStructDecl.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"
#include "compiler/ASTDiagnoser.h"
#include "std/except.h"

void GenericStructDecl::finalize_signature(ASTAllocator& allocator, StructDefinition* inst) {

    // copying the inherited types
    for(auto& inh : inst->inherited) {
        inh.type = inh.type.copy(allocator);
    }

    // copying the variables
    inst->copy_variables_in_place(allocator, inst);

    // finalizing the signature of functions (and copying other nodes)
    for(auto& node_ptr : inst->mut_evaluated_nodes()) {
        const auto node = node_ptr;
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                GenericFuncDecl::finalize_signature(allocator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                GenericFuncDecl::finalize_signature(allocator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            default:
                // copying other nodes
                node_ptr = node->copy(allocator);
                node_ptr->set_parent(inst);
                break;
        }
    }

}

void GenericStructDecl::finalize_body(ASTAllocator& allocator, StructDefinition* def) {

    for(const auto func : def->master_functions()) {
        GenericFuncDecl::finalize_body(allocator, func);
    }

}

StructDefinition* GenericStructDecl::register_generic_args(
    GenericInstantiatorAPI& instantiator,
    std::vector<TypeLoc>& generic_args
) {

    auto& container = instantiator.getContainer();
    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();
    auto& reg_mutex = instantiator.getRegistrationMutex();

    // locking the mutex to check (and maybe register) for generic instantiation
    reg_mutex.lock();

    const auto itr = register_generic_usage(allocator, this, container, generic_args, ((std::vector<void*>&) instantiations));
    if(!itr.second) {
        const auto idx = itr.first;
        reg_mutex.unlock();
        return instantiations[idx];
    }

    const auto impl = master_impl->shallow_copy(allocator);

    if(itr.first != instantiations.size()) {
#ifdef DEBUG
        CHEM_THROW_RUNTIME("not the index we expected");
#endif
    }

    impl->generic_parent = this;
    impl->generic_instantiation = itr.first;
    instantiations.emplace_back(impl);
    container.put_current_module_instantiation(impl);

    // finalize signature while holding the lock
    finalize_signature(allocator, impl);
    auto ptr = impl;
    const auto span = std::span<StructDefinition*>(&ptr, 1);
    instantiator.FinalizeSignature(this, span);

    // generate default functions
    impl->generate_functions(allocator, diagnoser, impl);

    // unlock after signature is finalized
    reg_mutex.unlock();

    // body finalization proceeds without the lock
    if(body_linked) {
        finalize_body(allocator, impl);
        instantiator.FinalizeBody(this, span);
    }

    return impl;

}

StructDefinition* GenericStructDecl::instantiate_type(
    GenericInstantiatorAPI& instantiator,
    std::vector<TypeLoc>& types,
    SourceLocation location
) {

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

#ifdef COMPILER_BUILD

void GenericStructDecl::code_gen_external_declare(Codegen &gen) {
    // only declare the instantiations that have been bodied
    unsigned i = 0;
    const auto total = total_bodied_instantiations;
    while(i < total) {
        instantiations[i++]->code_gen_external_declare(gen);
    }
}

#endif