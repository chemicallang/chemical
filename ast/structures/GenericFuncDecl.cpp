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

FunctionDeclaration* GenericFuncDecl::instantiate_call(
    GenericInstantiatorAPI& instantiator,
    FunctionCall* call,
    BaseType* expected_type
) {

    auto& container = instantiator.getContainer();
    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();

    const auto total = generic_params.size();
    std::vector<TypeLoc> generic_args(total, TypeLoc(nullptr));
    infer_generic_args(allocator, generic_args, generic_params, call, diagnoser, expected_type);
    // purify generic args, this is done if this call is inside a generic function
    // by calling pure we resolve that type to its specialized version
    // because this function runs in a loop, below the function 'register_indirect_generic_iteration' calls this
    // function on functions that registered as subscribers (generic calls were present inside this generic function)
    unsigned i = 0;
    while(i < generic_args.size()) {
        auto& type = generic_args[i];
        if(type) {
            type = {type->canonical(), type.getLocation()};
        }
        i++;
    }

    const auto itr = register_generic_usage(allocator, this, container, generic_args, ((std::vector<void*>&) instantiations));

    // this will only happen, when we probably couldn't infer the generic args
    if(itr.first == -1) {
        diagnoser.error("couldn't register generic instantiation", call);
        return nullptr;
    }
    // we activate the iteration just registered, because below we make call to register_indirect_iteration below
    // which basically calls register_call recursive on function calls present inside this function that are generic
    // which resolve specialized type using pure_type we called in the above loop
    // this function sets the iterations of the call_subscribers, however we haven't even
    // set their corresponding iterations in their subscribed map, we're doing it in the loop below
    // therefore we don't need to set generic iterations of subscribers
//    set_gen_itr_no_subs(itr.first);
    if(!itr.second) { // itr.second -> new iteration has been registered for which previously didn't exist

        // instantiation already exists
        return instantiations[itr.first];

    }

#ifdef DEBUG
    if(itr.first != instantiations.size()) {
        // TODO enable this error, currently when a type deduction fails, we expect the type to be specified in argument list
        if(itr.first < instantiations.size()) {
            return instantiations[itr.first];
        }
        throw std::runtime_error("iteration registered, that is not on the expected index");
    }
#endif

    const auto impl = master_impl->shallow_copy(allocator);

    impl->generic_parent = this;
    impl->generic_instantiation = (int) instantiations.size();
    instantiations.emplace_back(impl);

    if(body_linked) {

        // signature and body both have been linked for master_impl
        // so all we need to do is
        finalize_signature(allocator, impl);
        finalize_body(allocator, impl);

        // now finalize using instantiator
        auto ptr = impl;
        const auto span = std::span<FunctionDeclaration*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);
        instantiator.FinalizeBody(this, span);

//        // since signature has already been linked, we need to manually put into
//        // the appropriate struct for which this function is for
//        if(impl->isExtensionFn()) {
//            impl->put_as_extension_function(allocator, diagnoser);
//        }

    } else if(signature_linked) {

        // signature and body both have been linked for master_impl
        // so all we need to do is
        finalize_signature(allocator, impl);

        // now finalize using instantiator
        auto ptr = impl;
        const auto span = std::span<FunctionDeclaration*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);

//        // since signature has already been linked, we need to manually put into
//        // the appropriate struct for which this function is for
//        if(impl->isExtensionFn()) {
//            impl->put_as_extension_function(allocator, diagnoser);
//        }

    }

    return impl;

}

#ifdef COMPILER_BUILD

void GenericFuncDecl::code_gen_declare(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen_declare(gen);
        i++;
    }
}

void GenericFuncDecl::code_gen(Codegen &gen) {
    auto& i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen(gen);
        i++;
    }
}

void GenericFuncDecl::code_gen_external_declare(Codegen &gen) {
    unsigned i = 0;
    const auto total = total_bodied_instantiations;
    while(i < total) {
        instantiations[i]->code_gen_external_declare(gen);
        i++;
    }
}

#endif