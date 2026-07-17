#include "GenericFuncDecl.h"
#include "ast/utils/ASTUtils.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/GenericType.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/types/LinkedType.h"
#include "ast/values/FunctionCall.h"
#include "compiler/ASTDiagnoser.h"
#include "std/except.h"

void GenericFuncDecl::finalize_signature(ASTAllocator& allocator, FunctionDeclaration* decl) {

    // copying parameters
    for(auto& param : decl->params) {
        const auto copied = param->copy(allocator);
        copied->set_parent(decl);
        param = copied;
    }

    // copying return type
    decl->returnType = decl->returnType.copy(allocator);

}

void GenericFuncDecl::finalize_body(ASTAllocator& allocator, FunctionDeclaration* decl) {

    if(decl->body.has_value()) {
        for(auto& node : decl->body->nodes) {
            const auto copied = node->copy(allocator);
            copied->set_parent(decl);
            node = copied;
        }
    }

}

FunctionDeclaration* GenericFuncDecl::register_generic_args(
    GenericInstantiatorAPI& instantiator,
    std::vector<TypeLoc>& generic_args,
    SourceLocation location,
    InstantiationRequirement requirement
) {

    auto& container = instantiator.getContainer();
    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();
    auto& reg_mutex = instantiator.getRegistrationMutex();

    // locking the mutex to check (and maybe register) for generic instantiation
    reg_mutex.lock();

    // checking
    const auto itr = register_generic_usage(allocator, this, container, generic_args, ((std::vector<void*>&) instantiations));

    // this will only happen, when we probably couldn't infer the generic args
    if(itr.first == -1) {
        // unlocking mutex, because we failed to register
        reg_mutex.unlock();
        diagnoser.error("couldn't register generic instantiation", location);
        return nullptr;
    }

    if(!itr.second) { // existing instantiation — reuse the pointer
        const auto idx = itr.first;
        reg_mutex.unlock();
        // during body finalization, wait for the dependency's signature to be finalized
        if(requirement == InstantiationRequirement::SignatureFinalization) {
            instantiator.waitSignatureFinalized(this, idx);
        }
        return instantiations[idx];
    }

    if(itr.first != instantiations.size()) {
        // TODO enable this error, currently when a type deduction fails, we expect the type to be specified in argument list
        if(itr.first < instantiations.size()) {
            reg_mutex.unlock();
            return instantiations[itr.first];
        }
        CHEM_THROW_RUNTIME("iteration registered, that is not on the expected index");
    }

    const auto impl = master_impl->shallow_copy(allocator);

    impl->generic_parent = this;
    impl->generic_instantiation = (int) instantiations.size();
    instantiations.emplace_back(impl);
    container.put_current_module_instantiation(impl);

    // set status to Building before unlocking
    const auto inst_idx = itr.first;
    {
        std::lock_guard<std::mutex> status_lock(container.getInstantiationStatusMutex());
        instantiation_statuses.push_back({ InstantiationStatus::Registered, std::this_thread::get_id() });
    }

    // unlock reg_mutex immediately — signature finalization only needs registration
    reg_mutex.unlock();

    // finalize signature without holding the lock
    finalize_signature(allocator, impl);

    auto ptr = impl;
    const auto span = std::span<FunctionDeclaration*>(&ptr, 1);
    instantiator.FinalizeSignature(this, span);

    // mark signature as finalized and notify waiters
    instantiator.notifySignatureFinalized(this, inst_idx);

    // body finalization proceeds without the lock
    if(body_linked) {
        finalize_body(allocator, impl);
        instantiator.FinalizeBody(this, span);
    }

    return impl;
}

FunctionDeclaration* GenericFuncDecl::instantiate_call(
    GenericInstantiatorAPI& instantiator,
    FunctionCall* call,
    BaseType* expected_type,
    InstantiationRequirement requirement
) {

    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();

    std::vector<TypeLoc> generic_args;

    // initialize the generic args
    const auto success = initialize_generic_args(diagnoser, generic_args, generic_params, call->generic_list);
    if(!success) {
        return nullptr;
    }

    // infer the generic arguments
    infer_generic_args(allocator, generic_args, generic_params, call, diagnoser, expected_type);

    // check all types have been inferred
    const auto success2 = check_inferred_generic_args(diagnoser, generic_args, generic_params, call->encoded_location());
    if(!success2) {
        return nullptr;
    }

    // canonicalize the generic arguments
    // TODO: is there a need for this
    // TODO: if test suite passes without this, delete canonicalization of generic parameters
    unsigned i = 0;
    while(i < generic_args.size()) {
        auto& type = generic_args[i];
        if(type) {
            type = {type->canonical(), type.getLocation()};
        }
        i++;
    }

    return register_generic_args(instantiator, generic_args, call->encoded_location(), requirement);

}

#ifdef COMPILER_BUILD

void GenericFuncDecl::code_gen_external_declare(Codegen &gen) {
    unsigned i = 0;
    const auto total = total_bodied_instantiations;
    while(i < total) {
        instantiations[i]->code_gen_external_declare(gen);
        i++;
    }
}

#endif