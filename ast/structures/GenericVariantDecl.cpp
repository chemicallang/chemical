// Copyright (c) Chemical Language Foundation 2025.

#include "GenericVariantDecl.h"
#include "compiler/SymbolResolver.h"
#include "ast/utils/GenericUtils.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/GenericType.h"
#include "compiler/generics/GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"

void GenericVariantDecl::declare_top_level(SymbolResolver &linker, ASTNode *&node_ptr) {
    master_impl->generic_parent = this;
    linker.declare(master_impl->name_view(), this);
}

void GenericVariantDecl::finalize_signature(ASTAllocator& allocator, VariantDefinition* inst) {

    // copying the inherited types
    for(auto& inh : inst->inherited) {
        inh.type = inh.type->copy(allocator);
    }

    // copying the variables
    auto begin = inst->variables.begin();
    auto end = inst->variables.end();
    while(begin != end) {
        auto& variable_member = begin.value();
        const auto copied = variable_member->copy_member(allocator);
        copied->set_parent(inst);
        variable_member = copied;
        begin++;
    }

    // finalizing the signature of functions
    for(const auto func : inst->functions()) {
        // non generic functions inside struct must have active iteration zero
        // when we add generic functions support inside containers, we'll change this
        func->active_iteration = 0;
        GenericFuncDecl::finalize_signature(allocator, func);
    }

}

void GenericVariantDecl::finalize_body(ASTAllocator& allocator, VariantDefinition* def) {

    for(const auto func : def->functions()) {
        GenericFuncDecl::finalize_body(allocator, func);
    }

}

void GenericVariantDecl::link_signature(SymbolResolver &linker) {
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
        // since these instantiations were created before link_signature
        // they don't have any generated functions like default constructor / destructor
        // TODO we're passing the ast allocator, this generic could be at module level, in that case we should select the module alloctor
        inst->generate_functions(*linker.ast_allocator, linker);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeSignature(this, instantiations);
}

void GenericVariantDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
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

BaseType* GenericVariantDecl::create_value_type(ASTAllocator &allocator) {
    return master_impl->create_value_type(allocator);
}

VariantDefinition* GenericVariantDecl::register_generic_args(GenericInstantiatorAPI& instantiator, std::vector<BaseType*>& types) {

    const auto types_size = types.size();
    std::vector<BaseType*> generic_args(types_size);
    unsigned i = 0;
    for(auto& type : types) {
        generic_args[i] = type;
        i++;
    }

    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();

    // we return null when all types inferred aren't specialized
    if(!are_all_specialized(generic_args)) {
        return nullptr;
    }

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
        impl->generate_functions(allocator, diagnoser);

        // now finalize using instantiator
        auto ptr = impl;
        const auto span = std::span<VariantDefinition*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);
        instantiator.FinalizeBody(this, span);

    } else if(signature_linked) {

        // copy over the extension functions, if more functions were linked because of link_signature called upon function declarations
        // TODO unsure this line is needed
        // impl->extension_functions = master_impl->extension_functions;

        // signature and body both have been linked for master_impl
        // so all we need to do is
        finalize_signature(allocator, impl);
        impl->generate_functions(allocator, diagnoser);

        // now finalize using instantiator
        auto ptr = impl;
        const auto span = std::span<VariantDefinition*>(&ptr, 1);
        instantiator.FinalizeSignature(this, span);

    }

    return impl;

}

VariantDefinition* GenericVariantDecl::instantiate_call(GenericInstantiatorAPI& instantiator, FunctionCall* call, BaseType* expected_type) {

    auto& allocator = instantiator.getAllocator();
    auto& diagnoser = instantiator.getDiagnoser();

    const auto total = generic_params.size();
    std::vector<BaseType*> generic_args(total);

    // set all to default type (if default type is not present, it would automatically be nullptr)
    unsigned i = 0;
    while(i < total) {
        generic_args[i] = generic_params[i]->def_type;
        i++;
    }

    // set given generic args to generic parameters
    i = 0;
    for(auto& arg : call->generic_list) {
        generic_args[i] = arg;
        i++;
    }

    // infer args, if user gave less args than expected
    if(call->generic_list.size() != total) {
        call->infer_generic_args(diagnoser, generic_args);
    }

    // inferring type by expected type
    if(expected_type && expected_type->kind() == BaseTypeKind::Generic) {
        const auto type = ((GenericType*) expected_type);
        const auto linked = type->linked_node();
        if(linked && linked->kind() == ASTNodeKind::VariantDecl && linked->as_variant_def()->generic_parent == this) {
            i = 0;
            for(auto& arg : type->types) {
                generic_args[i] = arg;
                i++;
            }
        }
    }

    // check all types have been inferred
    i = 0;
    for(const auto arg : generic_args) {
        if(arg) {
//            if(arg->kind() == BaseTypeKind::Linked && arg->as_linked_type_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam) {
//                diagnoser.error(call) << "couldn't infer type for generic parameter at index " << std::to_string(i);
//            }
        } else {
            diagnoser.error(call) << "couldn't infer type for generic parameter at index " << std::to_string(i);
        }
        i++;
    }

    // registers the generic args
    return register_generic_args(instantiator, generic_args);

}

#ifdef COMPILER_BUILD

void GenericVariantDecl::code_gen_declare(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen_declare(gen);
        i++;
    }
}

void GenericVariantDecl::code_gen(Codegen &gen) {
    auto i = total_bodied_instantiations;
    const auto total = instantiations.size();
    while(i < total) {
        instantiations[i]->code_gen(gen);
        i++;
    }
    total_bodied_instantiations = instantiations.size();
}

void GenericVariantDecl::code_gen_external_declare(Codegen &gen) {
    // only declare the instantiations that have been bodied
    auto i = 0;
    while(i < total_bodied_instantiations) {
        instantiations[i]->code_gen_external_declare(gen);
        i++;
    }
}

#endif