
#include "GenericInstantiator.h"
#include "GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericStructDecl.h"

BaseType* GenericInstantiator::get_concrete_gen_type(BaseType* type) {
    if(type->kind() == BaseTypeKind::Linked){
        const auto linked_type = type->as_linked_type_unsafe();
        if(linked_type->linked->kind() == ASTNodeKind::GenericTypeParam) {
            return linked_type->linked->as_generic_type_param_unsafe()->concrete_type();
        }
    }
    return nullptr;
}

void GenericInstantiator::VisitFunctionCall(FunctionCall *call) {
    // visit, this would replace generic args and arguments of this call
    RecursiveVisitor<GenericInstantiator>::VisitFunctionCall(call);
    // now this call can be generic, in this case this call probably doesn't have an implementation
    // since current function is generic as well, let's check this
    // TODO passing nullptr as expected type
    call->instantiate_gen_call(allocator, diagnoser, nullptr);
}

void GenericInstantiator::VisitAccessChain(AccessChain* value) {
    RecursiveVisitor<GenericInstantiator>::VisitAccessChain(value);

    // relink first identifier in access chains
    const auto val = value->values.front();
    if(val->kind() == ValueKind::Identifier) {
        const auto id = val->as_identifier_unsafe();
        const auto node = table.resolve(id->value);
        if(node) {
            id->linked = node;
        }
    }

}

void GenericInstantiator::VisitLinkedType(LinkedType* type) {

    // relink the type if found
    const auto node = table.resolve(type->type);
    if(node) {
        type->linked = node;
    }

}

void GenericInstantiator::VisitGenericType(GenericType* type) {

    const auto referenced = type->referenced;
    auto& linked_ptr = referenced->linked;
    const auto linked = linked_ptr;
    if(linked->kind() == ASTNodeKind::GenericStructDecl) {
        if(linked == current_gen) {
            // self referential data structure
            linked_ptr = current_impl_ptr;
        } else {
            // relink generic struct decl with instantiated type
            linked_ptr = ((GenericStructDecl*) linked)->register_generic_args(allocator, diagnoser, type->types);
        }
    }
}

FunctionDeclaration* GenericInstantiator::Instantiate(GenericFuncDecl* decl, size_t itr) {

    // creating a shallow copy of the function
    const auto impl = decl->master_impl->copy(allocator);

    // set the curren gen decl
    current_gen = decl;

    // set implementation pointer
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // start a symbol scope
    table.scope_start();

    // replacing parameter types in the function
    // also declaring them as we must relink with copied nodes
    auto& params = impl->params;
    const auto total_params = params.size();
    auto i = 0;
    while(i < total_params) {
        visit(params[i]);
        table.declare(params[i]->name_view(), params[i]);
        i++;
    }

    // replace the return type
    visit(impl->returnType);

    // visiting the scope
    visit(impl->body.value());

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // end the symbol scope
    table.scope_end();

    // returning the new implementation
    return impl;

}

StructDefinition* GenericInstantiator::Instantiate(GenericStructDecl* decl, size_t itr) {

    const auto impl = decl->master_impl->copy(allocator);

    // this allows us to check self-referential pointers to generic decls
    current_gen = decl;

    // set implementation pointer
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // create a symbol scope
    table.scope_start();

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(auto& var : impl->variables) {
        table.declare(var.first, var.second);
        visit(var.second);
    }

    // declare function names before visiting
    for(const auto func : impl->functions()) {
        table.declare(func->name_view(), func);
    }

    // TODO generic functions inside a generic struct shouldn't be visited and instantiated

    // visiting functions
    for(const auto func : impl->functions()) {
        // relink return type of constructors with implementations
        if(func->is_constructor_fn()) {
            const auto linkedType = func->returnType->as_linked_type_unsafe();
            linkedType->linked = impl;
        }
        // replacing implicit parameters to self in functions
        for(const auto param : func->params) {
            if(param->is_implicit && param->name_view() == "self" || param->name_view() == "other") {
                // replacing very unsafely to save performance, as we always know it's going to be a ref to linked type
                param->type->as_reference_type_unsafe()->type->as_linked_type_unsafe()->linked = impl;
            }
        }
        visit(func);
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // returning the new implementation
    return impl;

}

// Generic Instantiator API

FunctionDeclaration* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericFuncDecl* decl, size_t itr) {
    auto inst = GenericInstantiator(astAllocator, diagnoser);
    return inst.Instantiate(decl, itr);
}

StructDefinition* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericStructDecl* decl, size_t itr) {
    auto inst = GenericInstantiator(astAllocator, diagnoser);
    return inst.Instantiate(decl, itr);
}