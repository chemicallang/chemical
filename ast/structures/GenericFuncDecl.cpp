#include "GenericFuncDecl.h"
#include "ast/utils/ASTUtils.h"
#include "ast/utils/GenericUtils.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "compiler/SymbolResolver.h"

void GenericFuncDecl::declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.declare(master_impl->name_view(), this);
}

void GenericFuncDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    // symbol resolve the master declaration
    // TODO this creates an extra scope
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    master_impl->link_signature(linker);
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    // we set it has usage, so every shallow copy or instantiation has usage
    master_impl->set_has_usage(true);
    linker.scope_end();
}

FunctionDeclaration* GenericFuncDecl::instantiate_call(
    SymbolResolver& resolver,
    FunctionCall* call,
    BaseType* expected_type
) {

    auto& diagnoser = resolver;
    auto& astAllocator = *resolver.ast_allocator;

    const auto total = generic_params.size();
    std::vector<BaseType*> generic_args(total);
    infer_generic_args(generic_args, generic_params, call, diagnoser, expected_type);
    // purify generic args, this is done if this call is inside a generic function
    // by calling pure we resolve that type to its specialized version
    // because this function runs in a loop, below the function 'register_indirect_generic_iteration' calls this
    // function on functions that registered as subscribers (generic calls were present inside this generic function)
    unsigned i = 0;
    while(i < generic_args.size()) {
        auto& type = generic_args[i];
        if(type) {
            type = type->pure_type();
        }
        i++;
    }
    const auto itr = register_generic_usage(astAllocator, generic_params, generic_args);
    // we activate the iteration just registered, because below we make call to register_indirect_iteration below
    // which basically calls register_call recursive on function calls present inside this function that are generic
    // which resolve specialized type using pure_type we called in the above loop
    // this function sets the iterations of the call_subscribers, however we haven't even
    // set their corresponding iterations in their subscribed map, we're doing it in the loop below
    // therefore we don't need to set generic iterations of subscribers
//    set_gen_itr_no_subs(itr.first);
    if(itr.second) { // itr.second -> new iteration has been registered for which previously didn't exist
        // TODO subscribers not done yet
//        for (auto sub: subscribers) {
//            sub->report_parent_usage(astAllocator, diagnoser, itr.first);
//        }
//        const auto parent_itr = get_parent_iteration();
//        for(auto call_sub : call_subscribers) {
//            const auto call_itr = call_sub.first->register_indirect_generic_iteration(astAllocator, diagnoser, call_sub.second);
//            // saving the call iteration into the map
//            gen_call_iterations[pack_gen_itr(parent_itr, itr.first)] = call_itr;
//        }
    } else {

        return instantiations[itr.first];

    }

    auto created = Instantiate(astAllocator, this, itr.first);

#ifdef DEBUG
    if(itr.first != instantiations.size()) {
        throw std::runtime_error("iteration registered, that is not on the expected index");
    }
#endif

    instantiations.emplace_back(created);

    return created;

}

#ifdef COMPILER_BUILD

void GenericFuncDecl::code_gen_declare(Codegen &gen) {
    for(const auto inst : instantiations) {
        inst->code_gen_declare(gen);
    }
}

void GenericFuncDecl::code_gen(Codegen &gen) {
    for(const auto inst : instantiations) {
        inst->code_gen(gen);
    }
}

#endif