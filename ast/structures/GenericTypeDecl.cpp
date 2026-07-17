// Copyright (c) Chemical Language Foundation 2025.

#include "GenericTypeDecl.h"
#include "ast/structures/GenericTypeParameter.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/ASTDiagnoser.h"
#include "std/except.h"
#include <thread>

void GenericTypeDecl::finalize_signature(ASTAllocator& allocator, TypealiasStatement* inst) {
    inst->actual_type = inst->actual_type.copy(allocator);
}

TypealiasStatement* GenericTypeDecl::copy_master(ASTAllocator& allocator) {
    // create a shallow copy
    const auto impl = master_impl->shallow_copy(allocator);
    impl->generic_parent = this;
    return impl;
}

TypealiasStatement* GenericTypeDecl::register_generic_args(
    GenericInstantiatorAPI& instantiator,
    std::vector<TypeLoc>& generic_args
) {

    auto& container = instantiator.getContainer();
    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();
    auto& reg_mutex = instantiator.getRegistrationMutex();
    auto& statuses = instantiation_statuses;
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
            if(idx < statuses.size() && statuses[idx].status == InstantiationStatus::Building
               && statuses[idx].builder_thread != std::this_thread::get_id()) {
                instantiator.waitInstantiationFinalized(lock, this, idx);
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

    // signature and body both have been linked for master_impl
    // so all we need to do is
    finalize_signature(allocator, impl);

    // now finalize using instantiator
    auto ptr = impl;
    const auto span = std::span<TypealiasStatement*>(&ptr, 1);
    instantiator.FinalizeSignature(this, span);

    // mark as finalized and wake any waiters
    {
        std::lock_guard<std::mutex> lock(status_mutex);
        statuses[inst_idx].status = InstantiationStatus::Finalized;
    }
    status_cv.notify_all();

    return impl;

}

TypealiasStatement* GenericTypeDecl::instantiate_type(
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