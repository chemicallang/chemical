
#include "GenericInstantiator.h"
#include "GenInstantiatorAPI.h"

BaseType* get_concrete_gen_type(BaseType* type) {
    if(type->kind() == BaseTypeKind::Linked){
        const auto linked_type = type->as_linked_type_unsafe();
        if(linked_type->linked->kind() == ASTNodeKind::GenericTypeParam) {
            return linked_type->linked->as_generic_type_param_unsafe()->concrete_type();
        }
    }
    return nullptr;
}

void GenericInstantiator::VisitFunctionParam(FunctionParam *param) {
    if(param->type->kind() == BaseTypeKind::Linked){
        const auto linked_type = param->type->as_linked_type_unsafe();
        if(linked_type->linked->kind() == ASTNodeKind::GenericTypeParam) {
            const auto new_param = param->shallow_copy(allocator);
            new_param->type = linked_type->linked->as_generic_type_param_unsafe()->concrete_type();
            *replacement_pointer = new_param;
        }
    }
}

FunctionDeclaration* GenericInstantiator::Instantiate(GenericFuncDecl* decl, size_t itr) {

    // creating a shallow copy of the function
    const auto impl = decl->master_impl->shallow_copy(allocator);

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
    impl->returnType = get_concrete_gen_type(impl->returnType);

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->active_iteration = -1;
    }

    // visiting the scope
    visit(impl->body.value());

    // returning the new implementation
    return impl;

}

// Generic Instantiator API

FunctionDeclaration* Instantiate(ASTAllocator& astAllocator, GenericFuncDecl* decl, size_t itr) {

    auto inst = GenericInstantiator(astAllocator);

    return inst.Instantiate(decl, itr);

}