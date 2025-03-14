// Copyright (c) Chemical Language Foundation 2025.

#include "GenericTypeDecl.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/GenericTypeParameter.h"
#include "ast/utils/GenericUtils.h"

void GenericTypeDecl::declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.declare(master_impl->name_view(), this);
}

void GenericTypeDecl::finalize_signature(ASTAllocator& allocator, TypealiasStatement* inst) {
    inst->actual_type = inst->actual_type->copy(allocator);
}

void GenericTypeDecl::link_signature(SymbolResolver &linker) {
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

BaseType* GenericTypeDecl::create_value_type(ASTAllocator &allocator) {
    return master_impl->create_value_type(allocator);
}

TypealiasStatement* GenericTypeDecl::register_generic_args(GenericInstantiatorAPI& instantiator, std::vector<BaseType*>& types) {

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