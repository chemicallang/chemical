
#include "GenericInstantiator.h"
#include "GenInstantiatorAPI.h"

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

FunctionDeclaration* GenericInstantiator::Instantiate(GenericFuncDecl* decl, size_t itr) {

    // creating a shallow copy of the function
    const auto impl = decl->master_impl->copy(allocator);

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        // TODO cast to int32
        param->active_iteration = (int16_t) itr;
    }

    // replacing parameter types in the function
    auto& params = impl->params;
    const auto total_params = params.size();
    auto i = 0;
    while(i < total_params) {
        visit(params[i]);
        i++;
    }

    // replace the return type
    visit(impl->returnType);

    // visiting the scope
    visit(impl->body.value());

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->active_iteration = -1;
    }

    // returning the new implementation
    return impl;

}

// Generic Instantiator API

FunctionDeclaration* Instantiate(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, GenericFuncDecl* decl, size_t itr) {

    auto inst = GenericInstantiator(astAllocator, diagnoser);

    return inst.Instantiate(decl, itr);

}