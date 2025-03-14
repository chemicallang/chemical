
#include "GenericInstantiator.h"
#include "GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericImplDecl.h"

BaseType* GenericInstantiator::get_concrete_gen_type(BaseType* type) {
    if(type->kind() == BaseTypeKind::Linked){
        const auto linked = type->as_linked_type_unsafe()->linked;
        if(linked->kind() == ASTNodeKind::GenericTypeParam) {
            const auto ty = linked->as_generic_type_param_unsafe()->concrete_type();
#ifdef DEBUG
            if(ty == nullptr) {
                throw std::runtime_error("generic active type doesn't exist");
            }
            if(ty->kind() == BaseTypeKind::Linked && ty->linked_node()->kind() == ASTNodeKind::GenericTypeParam) {
                throw std::runtime_error("unexpected generic type parameter usage");
            }
#endif
            return ty;
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
    GenericInstantiator instantiator(allocator, diagnoser);
    GenericInstantiatorAPI genApi(&instantiator);
    if(!call->instantiate_gen_call(genApi, nullptr)) {
        diagnoser.error("couldn't instantiate call", call);
    }
}

bool GenericInstantiator::relink_identifier(VariableIdentifier* val) const {
    const auto id = val->as_identifier_unsafe();
    const auto node = table.resolve(id->value);
    if(node) {
        id->linked = node;
        return true;
    } else {
        return false;
    }
}

void GenericInstantiator::VisitAccessChain(AccessChain* value) {

    // NOTE:
    // we handle the identifier manually BECAUSE during parsing
    // we may create a single identifier, which is not wrapped in an access chain
    // if it's just a single identifier it's always handled via VisitVariableIdentifier
    // however if it's in access chain, we need to handle it here and not visit it again

    // relink first identifier in access chains
    const auto val = value->values.front();
    if(val->kind() == ValueKind::Identifier) {
        if(relink_identifier(val->as_identifier_unsafe())) {
            // since parent has changed, we must relink the chain
            value->relink_parent();
        }
    } else {
        // since it's not a identifier, we must visit it
        visit(val);
    }

    // we do not need to visit values except the first one in access chain
    // this is because, after the first value, only identifiers are present
    // however we do not need to relink identifiers, if we did visit identifiers
    // it would relink identifiers with any symbols found which would break everything

}

void GenericInstantiator::VisitLinkedType(LinkedType* type) {

    // relink the type if found
    const auto node = table.resolve(type->type);
    if(node) {
        type->linked = node;
    }

}

void GenericInstantiator::VisitStructValue(StructValue *val) {
    RecursiveVisitor<GenericInstantiator>::VisitStructValue(val);
    const auto linked = val->linked_extendable();
    if(linked && linked->generic_parent != nullptr && linked->generic_instantiation == -1) {
        // we can see that this container is generic and it's the master implementation
        // we only give master implementation generic instantiation equal to -1
        // so now we will specialize it, the above recursive visitor must have already replaced the refType
        GenericInstantiator instantiator(allocator, diagnoser);
        GenericInstantiatorAPI genApi(&instantiator);
        val->resolve_container(genApi);
    }
}

void GenericInstantiator::VisitGenericType(GenericType* type) {

    // we do this manually, first we replace any generic arguments given to this generic type
    for(auto& t : type->types) {
        visit(t);
    }

    const auto referenced = type->referenced;
    auto& linked_ptr = referenced->linked;
    const auto linked = linked_ptr;
    switch(linked->kind()) {
        case ASTNodeKind::GenericStructDecl:
            if(linked == current_gen) {
                // self referential data structure
                linked_ptr = current_impl_ptr;
            } else {
                // relink generic struct decl with instantiated type
                GenericInstantiator instantiator(allocator, diagnoser);
                GenericInstantiatorAPI genApi(&instantiator);
                linked_ptr = linked->as_gen_struct_def_unsafe()->register_generic_args(genApi, type->types);
            }
        case ASTNodeKind::GenericUnionDecl:
            if(linked == current_gen) {
                // self referential data structure
                linked_ptr = current_impl_ptr;
            } else {
                // relink generic struct decl with instantiated type
                GenericInstantiator instantiator(allocator, diagnoser);
                GenericInstantiatorAPI genApi(&instantiator);
                linked_ptr = linked->as_gen_union_decl_unsafe()->register_generic_args(genApi, type->types);
            }
        case ASTNodeKind::GenericInterfaceDecl:
            if(linked == current_gen) {
                // self referential data structure
                linked_ptr = current_impl_ptr;
            } else {
                // relink generic struct decl with instantiated type
                GenericInstantiator instantiator(allocator, diagnoser);
                GenericInstantiatorAPI genApi(&instantiator);
                linked_ptr = linked->as_gen_interface_decl_unsafe()->register_generic_args(genApi, type->types);
            }
        case ASTNodeKind::GenericVariantDecl:
            if(linked == current_gen) {
                // self referential data structure
                linked_ptr = current_impl_ptr;
            } else {
                // relink generic struct decl with instantiated type
                GenericInstantiator instantiator(allocator, diagnoser);
                GenericInstantiatorAPI genApi(&instantiator);
                linked_ptr = linked->as_gen_variant_decl_unsafe()->register_generic_args(genApi, type->types);
            }
        case ASTNodeKind::GenericTypeDecl:
            if(linked == current_gen) {
                // TODO figure out if this works for when generic params of the parent have been used with the same type differently
                // TODO we may need to check that generic types are using the same exact parameters before doing this
                // self referential data structure
                linked_ptr = current_impl_ptr;
            } else {
                // relink generic struct decl with instantiated type
                GenericInstantiator instantiator(allocator, diagnoser);
                GenericInstantiatorAPI genApi(&instantiator);
                linked_ptr = linked->as_gen_type_decl_unsafe()->register_generic_args(genApi, type->types);
            }
        default:
            // we only visit the linked type in this case
            visit(type->referenced);
            break;
    }
}

void GenericInstantiator::Clear() {
    table.clear();
}

void GenericInstantiator::FinalizeSignature(TypealiasStatement* impl) {
    // this allows us to check self-referential pointers to generic decls
    const auto prev_impl = current_impl_ptr;
    // set implementation pointer
    current_impl_ptr = impl;
    // replace the return type
    visit(impl->actual_type);
    // reset the pointers
    current_impl_ptr = prev_impl;
}

void GenericInstantiator::FinalizeSignature(GenericTypeDecl* decl, TypealiasStatement* impl, size_t itr) {

    // this allows us to check self-referential pointers to generic decls
    current_gen = decl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // finalize the signature of typealias instantiation
    FinalizeSignature(impl);

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset the pointers
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void GenericInstantiator::FinalizeSignature(FunctionDeclaration* impl) {
    const auto prev_impl = current_impl_ptr;
    // set implementation pointer
    current_impl_ptr = impl;

    // replacing parameter types in the function
    for(auto& param : impl->params) {
        visit(param);
    }

    // replace the return type
    visit(impl->returnType);

    // reset the pointers
    current_impl_ptr = prev_impl;
}

void GenericInstantiator::FinalizeSignature(GenericFuncDecl* decl, FunctionDeclaration* impl, size_t itr) {

    // this allows us to check self-referential pointers to generic decls
    current_gen = decl;
    // set implementation pointer
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // finalize signature of the function
    FinalizeSignature(impl);

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset the pointers
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void GenericInstantiator::FinalizeBody(GenericFuncDecl* decl, FunctionDeclaration* impl, size_t itr) {

    // this allows us to check self-referential pointers to generic decls
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // visit the body if exists
    if(impl->body.has_value()) {
        // start a symbol scope
        table.scope_start();

        // declaring them as we must relink with copied nodes
        for(const auto param : impl->params) {
            table.declare(param->name_view(), param);
        }

        // visit the function body
        visit(impl->body.value());

        // end the body scope
        table.scope_end();
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset the pointers
    current_gen = nullptr;
    current_impl_ptr = nullptr;


}

void GenericInstantiator::FinalizeSignature(GenericStructDecl* decl, StructDefinition* impl, size_t itr) {

    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(auto& var : impl->variables()) {
        visit(var);
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
            if(param->is_implicit() && param->name_view() == "self" || param->name_view() == "other") {
                // replacing very unsafely to save performance, as we always know it's going to be a ref to linked type
                param->type->as_reference_type_unsafe()->type->as_linked_type_unsafe()->linked = impl;
            }
        }
        // finalize the signature of functions
        FinalizeSignature(func);
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void GenericInstantiator::FinalizeBody(GenericStructDecl* decl, StructDefinition* impl, size_t itr) {

    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for(const auto func : impl->functions()) {
        table.declare(func->name_view(), func);
    }

    // TODO generic functions inside a generic struct shouldn't be visited and instantiated

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->functions()) {
        if(func->body.has_value()) {
            // start scope for function body
            table.scope_start();

            // declare function parameters
            for(const auto param : func->params) {
                table.declare(param->name_view(), param);
            }

            // visit the body
            visit(func->body.value());

            // end the body scope
            table.scope_end();
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void GenericInstantiator::FinalizeSignature(GenericUnionDecl* decl, UnionDef* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(const auto var : impl->variables()) {
        visit(var);
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
            if(param->is_implicit() && param->name_view() == "self" || param->name_view() == "other") {
                // replacing very unsafely to save performance, as we always know it's going to be a ref to linked type
                param->type->as_reference_type_unsafe()->type->as_linked_type_unsafe()->linked = impl;
            }
        }
        // finalize the signature of functions
        FinalizeSignature(func);
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void GenericInstantiator::FinalizeBody(GenericUnionDecl* decl, UnionDef* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for(const auto func : impl->functions()) {
        table.declare(func->name_view(), func);
    }

    // TODO generic functions inside a generic struct shouldn't be visited and instantiated

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->functions()) {
        if(func->body.has_value()) {
            // start scope for function body
            table.scope_start();

            // declare function parameters
            for(const auto param : func->params) {
                table.declare(param->name_view(), param);
            }

            // visit the body
            visit(func->body.value());

            // end the body scope
            table.scope_end();
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;
}

void GenericInstantiator::FinalizeSignature(GenericInterfaceDecl* decl, InterfaceDefinition* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(const auto var : impl->variables()) {
        visit(var);
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
            if(param->is_implicit() && param->name_view() == "self" || param->name_view() == "other") {
                // replacing very unsafely to save performance, as we always know it's going to be a ref to linked type
                param->type->as_reference_type_unsafe()->type->as_linked_type_unsafe()->linked = impl;
            }
        }
        // finalize the signature of functions
        FinalizeSignature(func);
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void GenericInstantiator::FinalizeBody(GenericInterfaceDecl* decl, InterfaceDefinition* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for(const auto func : impl->functions()) {
        table.declare(func->name_view(), func);
    }

    // TODO generic functions inside a generic struct shouldn't be visited and instantiated

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->functions()) {
        if(func->body.has_value()) {
            // start scope for function body
            table.scope_start();

            // declare function parameters
            for(const auto param : func->params) {
                table.declare(param->name_view(), param);
            }

            // visit the body
            visit(func->body.value());

            // end the body scope
            table.scope_end();
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;
}

void GenericInstantiator::FinalizeSignature(GenericVariantDecl* decl, VariantDefinition* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(auto& var : impl->variables()) {
        visit(var);
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
            if(param->is_implicit() && param->name_view() == "self" || param->name_view() == "other") {
                // replacing very unsafely to save performance, as we always know it's going to be a ref to linked type
                param->type->as_reference_type_unsafe()->type->as_linked_type_unsafe()->linked = impl;
            }
        }
        // finalize the signature of functions
        FinalizeSignature(func);
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void GenericInstantiator::FinalizeBody(GenericVariantDecl* decl, VariantDefinition* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for(const auto func : impl->functions()) {
        table.declare(func->name_view(), func);
    }

    // TODO generic functions inside a generic struct shouldn't be visited and instantiated

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->functions()) {
        if(func->body.has_value()) {
            // start scope for function body
            table.scope_start();

            // declare function parameters
            for(const auto param : func->params) {
                table.declare(param->name_view(), param);
            }

            // visit the body
            visit(func->body.value());

            // end the body scope
            table.scope_end();
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;
}

void GenericInstantiator::FinalizeSignature(GenericImplDecl* decl, ImplDefinition* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(auto& var : impl->variables()) {
        visit(var);
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
            if(param->is_implicit() && param->name_view() == "self" || param->name_view() == "other") {
                // replacing very unsafely to save performance, as we always know it's going to be a ref to linked type
                param->type->as_reference_type_unsafe()->type->as_linked_type_unsafe()->linked = impl;
            }
        }
        // finalize the signature of functions
        FinalizeSignature(func);
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;
}

void GenericInstantiator::FinalizeBody(GenericImplDecl* decl, ImplDefinition* impl, size_t itr) {
    // set the pointers to gen decl and impl
    current_gen = decl;
    current_impl_ptr = impl;

    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->set_active_iteration((int) itr);
    }

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for(const auto func : impl->functions()) {
        table.declare(func->name_view(), func);
    }

    // TODO generic functions inside a generic struct shouldn't be visited and instantiated

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->functions()) {
        if(func->body.has_value()) {
            // start scope for function body
            table.scope_start();

            // declare function parameters
            for(const auto param : func->params) {
                table.declare(param->name_view(), param);
            }

            // visit the body
            visit(func->body.value());

            // end the body scope
            table.scope_end();
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        param->deactivate_iteration();
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;
}

void GenericInstantiator::FinalizeSignature(GenericTypeDecl* decl, const std::span<TypealiasStatement*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
    }
}

void GenericInstantiator::FinalizeSignature(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
    }
}

void GenericInstantiator::FinalizeBody(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeBody(decl, inst, inst->generic_instantiation);
        Clear();
    }
}

void GenericInstantiator::FinalizeSignature(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
    }
}

void GenericInstantiator::FinalizeBody(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeBody(decl, inst, inst->generic_instantiation);
        Clear();
    }
}

void GenericInstantiator::FinalizeSignature(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
    }
}

void GenericInstantiator::FinalizeBody(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeBody(decl, inst, inst->generic_instantiation);
        Clear();
    }
}

void GenericInstantiator::FinalizeSignature(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
    }
}

void GenericInstantiator::FinalizeBody(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeBody(decl, inst, inst->generic_instantiation);
        Clear();
    }
}

void GenericInstantiator::FinalizeSignature(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
    }
}

void GenericInstantiator::FinalizeBody(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeBody(decl, inst, inst->generic_instantiation);
        Clear();
    }
}

void GenericInstantiator::FinalizeSignature(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
    }
}

void GenericInstantiator::FinalizeBody(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeBody(decl, inst, inst->generic_instantiation);
        Clear();
    }
}

// Generic Instantiator API

GenericInstantiatorAPI::GenericInstantiatorAPI(
    ASTAllocator& astAllocator,
    ASTDiagnoser& diagnoser
) : giPtr(new GenericInstantiator(astAllocator, diagnoser)) {

}

GenericInstantiatorAPI::~GenericInstantiatorAPI() {
    if(owns) {
        delete giPtr;
    }
}

ASTAllocator& GenericInstantiatorAPI::getAllocator() {
    return giPtr->allocator;
}

ASTDiagnoser& GenericInstantiatorAPI::getDiagnoser() {
    return giPtr->diagnoser;
}

void GenericInstantiatorAPI::FinalizeSignature(GenericTypeDecl* decl, const std::span<TypealiasStatement*>& instantiations) {
    giPtr->FinalizeSignature(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeSignature(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations) {
    giPtr->FinalizeSignature(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeBody(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations) {
    giPtr->FinalizeBody(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeSignature(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations) {
    giPtr->FinalizeSignature(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeBody(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations) {
    giPtr->FinalizeBody(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeSignature(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations) {
    giPtr->FinalizeSignature(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeBody(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations) {
    giPtr->FinalizeBody(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeSignature(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations) {
    giPtr->FinalizeSignature(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeBody(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations) {
    giPtr->FinalizeBody(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeSignature(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations) {
    giPtr->FinalizeSignature(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeBody(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations) {
    giPtr->FinalizeBody(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeSignature(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations) {
    giPtr->FinalizeSignature(decl, instantiations);
}

void GenericInstantiatorAPI::FinalizeBody(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations) {
    giPtr->FinalizeBody(decl, instantiations);
}