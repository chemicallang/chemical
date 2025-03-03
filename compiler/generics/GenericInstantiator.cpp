
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

inline void replace_gen_type(BaseType*& type_ref) {
    const auto concrete = GenericInstantiator::get_concrete_gen_type(type_ref);
    if(concrete) {
        type_ref = concrete;
    }
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
    replace_gen_type(impl->returnType);

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