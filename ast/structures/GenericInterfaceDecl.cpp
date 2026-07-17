// Copyright (c) Chemical Language Foundation 2025.

#include "GenericInterfaceDecl.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"
#include "std/except.h"
#include <thread>

void GenericInterfaceDecl::finalize_signature(ASTAllocator& allocator, InterfaceDefinition* inst) {

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

void GenericInterfaceDecl::finalize_body(ASTAllocator& allocator, InterfaceDefinition* def) {

    for(const auto func : def->master_functions()) {
        GenericFuncDecl::finalize_body(allocator, func);
    }

}

InterfaceDefinition* GenericInterfaceDecl::register_generic_args(
    GenericInstantiatorAPI& instantiator,
    std::vector<TypeLoc>& generic_args
) {

    auto& container = instantiator.getContainer();
    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();
    auto& reg_mutex = instantiator.getRegistrationMutex();
    auto& statuses = instantiator.getInstantiationStatuses(this);
    auto& status_mutex = instantiator.getInstantiationStatusMutex();
    auto& status_cv = instantiator.getInstantiationStatusCV();

    // locking the mutex to check (and maybe register) for generic instantiation
    reg_mutex.lock();

    const auto itr = register_generic_usage(allocator, this, container, generic_args, ((std::vector<void*>&) instantiations));
    if(!itr.second) {
        const auto idx = itr.first;
        // unlock registration mutex
        reg_mutex.unlock();

        // wait for finalization if still building
        {
            std::unique_lock<std::mutex> lock(status_mutex);
            if(idx < statuses.size() && statuses[idx].status == InstantiationStatus::Building) {
                if(!instantiator.isBuildingThread(this, idx, std::this_thread::get_id())) {
                    instantiator.waitInstantiationFinalized(lock, this, idx);
                }
            }
        }
        return instantiations[idx];
    }

    // we will do a complete instantiation right now
    const auto impl = master_impl->shallow_copy(allocator);

    if(itr.first != instantiations.size()) {
#ifdef DEBUG
        CHEM_THROW_RUNTIME("not the index we expected");
#endif
    }

    const auto inst_idx = itr.first;

    // mark status as Building before unlocking registration mutex
    statuses.push_back({InstantiationStatus::Building, std::this_thread::get_id()});

    // must set this variables, otherwise finalization won't be able to get which concrete implementation to use
    impl->generic_parent = this;
    impl->generic_instantiation = itr.first;
    // store the pointer of instantiation
    instantiations.emplace_back(impl);
    container.put_current_module_instantiation(impl);

    // unlocking the mutex because we registered an instantiation
    // (other threads would find this from instantiations vector using an index
    reg_mutex.unlock();

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

    } else {

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

    // mark as finalized and wake any waiters
    {
        std::lock_guard<std::mutex> lock(status_mutex);
        statuses[inst_idx].status = InstantiationStatus::Finalized;
    }
    status_cv.notify_all();

    return impl;

}

InterfaceDefinition* GenericInterfaceDecl::instantiate_type(
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