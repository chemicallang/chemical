
#include "GenericInstantiator.h"

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

FunctionDeclaration* GenericInstantiator::Instantiate(GenericFuncDecl* decl, std::vector<BaseType*>& gen_args) {

    // creating a shallow copy of the function
    const auto impl = decl->master_impl->shallow_copy(allocator);

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

    // returning the new implementation
    return impl;

}