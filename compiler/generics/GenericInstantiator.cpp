
#include "GenericInstantiator.h"
#include "GenInstantiatorAPI.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/values/TypeInsideValue.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericImplDecl.h"

void GenericInstantiator::make_gen_type_concrete(BaseType*& type) {
    if(type->kind() == BaseTypeKind::Linked){
        const auto linked = type->as_linked_type_unsafe()->linked;
        switch(linked->kind()) {
            case ASTNodeKind::GenericTypeParam: {
                const auto ty = linked->as_generic_type_param_unsafe()->concrete_type();
#ifdef DEBUG
                if(ty == nullptr) {
                    throw std::runtime_error("generic active type doesn't exist");
                }
                if(ty->kind() == BaseTypeKind::Linked && ty->as_linked_type_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam) {
                    throw std::runtime_error("unexpected generic type parameter usage");
                }
#endif
                type = ty;
                return;
            }
            case ASTNodeKind::GenericStructDecl:
            case ASTNodeKind::GenericUnionDecl:
            case ASTNodeKind::GenericInterfaceDecl:
            case ASTNodeKind::GenericVariantDecl:
            case ASTNodeKind::GenericTypeDecl:{
                if(linked == current_gen) {
                    type->as_linked_type_unsafe()->linked = current_impl_ptr;
                }
                return;
            }
            default:
                return;
        }
    }
}

void GenericInstantiator::VisitFunctionCall(FunctionCall *call) {
    // visit, this would replace generic args and arguments of this call
    RecursiveVisitor<GenericInstantiator>::VisitFunctionCall(call);
    // now this call can be generic, in this case this call probably doesn't have an implementation
    // since current function is generic as well, let's check this
    // TODO passing nullptr as expected type
    GenericInstantiator instantiator(container, getAllocator(), diagnoser);
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

    // NOTE:
    // we do not need to visit values except the first one in access chain
    // this is because, after the first value, only identifiers are present
    // however we do not need to relink identifiers, if we did visit identifiers
    // it would relink identifiers with any symbols found which would break everything

}

void GenericInstantiator::VisitLinkedType(LinkedType* type) {

    // relink the type if found
    const auto node = table.resolve(type->linked_name());
    if(node) {
        type->linked = node;
    }

}

void GenericInstantiator::VisitPatternMatchExpr(PatternMatchExpr* value) {
    RecursiveVisitor<GenericInstantiator>::VisitPatternMatchExpr(value);

    const auto member = value->find_member_from_expr(getAllocator(), diagnoser);
    if(member == nullptr) {
        return;
    }

    // set the member
    value->member = member;

    // declare the members so variables are relinked for generics
    auto& params = member->values;
    for(const auto nameId : value->param_names) {
        auto found = params.find(nameId->identifier);
        if(found == params.end()) {
            diagnoser.error(nameId) << "couldn't find parameter in variant member";
        } else {
            nameId->member_param = found->second;
            // we declare this id, so anyone can link with it
            table.declare(nameId->identifier, nameId);
        }
    }

}

void GenericInstantiator::VisitStructValue(StructValue *val) {
    RecursiveVisitor<GenericInstantiator>::VisitStructValue(val);
    const auto linked = val->linked_extendable();
    if(linked && linked->generic_parent != nullptr && linked->generic_instantiation == -1) {
        // we can see that this container is generic and it's the master implementation
        // we only give master implementation generic instantiation equal to -1
        // so now we will specialize it, the above recursive visitor must have already replaced the refType
        GenericInstantiator instantiator(container, getAllocator(), diagnoser);
        GenericInstantiatorAPI genApi(&instantiator);
        val->resolve_container(genApi);
    }
}

void GenericInstantiator::VisitSwitchStmt(SwitchStatement* stmt) {
    visit_it(stmt->expression);
    const auto varDef = stmt->getVarDefFromExpr();
    if(varDef == nullptr) {
        for (auto& thing: stmt->cases) {
            visit_it(thing.first);
        }
        for (auto& scope: stmt->scopes) {
            visit_it(scope);
        }
        return;
    }

    // we must switch the pointers to variant definition properly
    for(auto& aCase : stmt->cases) {
        if(aCase.first->kind() == ValueKind::VariantCase) {
            const auto varCase = aCase.first->as_variant_case_unsafe();
            const auto newChild = varDef->child(varCase->member->name);
            if(newChild && newChild->kind() == ASTNodeKind::VariantMember) {
                const auto newMem = newChild->as_variant_member_unsafe();
                varCase->member = newMem;
                for(const auto caseVar : varCase->identifier_list) {
                    auto foundNewVar = newMem->values.find(caseVar->name);
                    if(foundNewVar != newMem->values.end()) {
                        caseVar->member_param = foundNewVar->second;
                    } else {
                        diagnoser.error(aCase.first) << "couldn't find variant member by name '" << caseVar->name << "' during generic instantiation";
                    }
                }
            } else {
                diagnoser.error(aCase.first) << "couldn't find variant member by name '" << varCase->member->name << "' during generic instantiation";
            }
        }
    }

    // now when linking scopes, we must declare the variant case variables
    // so users (identifiers) point to new variant case variables
    int i = 0;
    const auto total = stmt->scopes.size();
    while(i < total) {
        auto& scope = stmt->scopes[i];

        // start a scope for variant case variables
        table.scope_start();

        // get and declare variant case variables
        for(auto& aCase : stmt->cases) {
            if(aCase.first->kind() == ValueKind::VariantCase && aCase.second == i) {
                const auto variantCase = aCase.first->as_variant_case_unsafe();
                for(const auto caseVar : variantCase->identifier_list) {
                    table.declare(caseVar->name, caseVar);
                }
            }
        }

        // linking the scope
        visit_it(scope);

        // end the scope to delete all the variant case variables
        table.scope_end();

        i++;
    }

}

/**
 * what it checks is whether the argument types are linked with given generic types parameters in order
 */
bool are_types_generic(std::vector<TypeLoc>& arg_types, std::vector<GenericTypeParameter*>& type_params) {
    if(arg_types.size() != type_params.size()) {
        return false;
    }
    unsigned i = 0;
    const auto total = arg_types.size();
    while(i < total) {
        const auto arg_type = arg_types[i];
        const auto param = type_params[i];
        if(arg_type->kind() != BaseTypeKind::Linked || arg_type->as_linked_type_unsafe()->linked != param) {
            return false;
        }
        i++;
    }
    return true;
}

void GenericInstantiator::VisitGenericType(GenericType* type) {

    // we do this manually, first we replace any generic arguments given to this generic type
    for(auto& t : type->types) {
        visit(t);
    }

    // NOTE: visiting the referenced type in this case would lead to a problem
    // the problem is that LinkedType visiting method, currently checks for linkage with a generic decl
    // and replaced with current implementation pointer, however user could be doing struct Point<T> { var next : Point<int> }
    // in this case another instantiation of Point is being made with different generic arguments, so we must not use current implementation
    // so we must not visit the linked type, if it's linked with any generic decl

    const auto referenced = type->referenced;
    auto& linked_ptr = referenced->linked;
    const auto linked = linked_ptr;
    switch(linked->kind()) {
        case ASTNodeKind::GenericStructDecl: {
            if (linked == current_gen) {
                if (are_types_generic(type->types, current_gen->generic_params)) {
                    // self referential data structure
                    linked_ptr = current_impl_ptr;
                    return;
                }
            }
            // relink generic struct decl with instantiated type
            GenericInstantiator instantiator(container, getAllocator(), diagnoser);
            GenericInstantiatorAPI genApi(&instantiator);
            linked_ptr = linked->as_gen_struct_def_unsafe()->instantiate_type(genApi, type->types);
            return;
        }
        case ASTNodeKind::GenericUnionDecl: {
            if (linked == current_gen) {
                if (are_types_generic(type->types, current_gen->generic_params)) {
                    // self referential data structure
                    linked_ptr = current_impl_ptr;
                    return;
                }
            }
            // relink generic struct decl with instantiated type
            GenericInstantiator instantiator(container, getAllocator(), diagnoser);
            GenericInstantiatorAPI genApi(&instantiator);
            linked_ptr = linked->as_gen_union_decl_unsafe()->instantiate_type(genApi, type->types);
            return;
        }
        case ASTNodeKind::GenericInterfaceDecl:{
            if(linked == current_gen) {
                if (are_types_generic(type->types, current_gen->generic_params)) {
                    // self referential data structure
                    linked_ptr = current_impl_ptr;
                    return;
                }
            }
            // relink generic struct decl with instantiated type
            GenericInstantiator instantiator(container, getAllocator(), diagnoser);
            GenericInstantiatorAPI genApi(&instantiator);
            linked_ptr = linked->as_gen_interface_decl_unsafe()->instantiate_type(genApi, type->types);
            return;
        }
        case ASTNodeKind::GenericVariantDecl:{
            if(linked == current_gen) {
                // self referential data structure
                if (are_types_generic(type->types, current_gen->generic_params)) {
                    // self referential data structure
                    linked_ptr = current_impl_ptr;
                    return;
                }
            }
            // relink generic struct decl with instantiated type
            GenericInstantiator instantiator(container, getAllocator(), diagnoser);
            GenericInstantiatorAPI genApi(&instantiator);
            linked_ptr = linked->as_gen_variant_decl_unsafe()->instantiate_type(genApi, type->types);
            return;
        }
        case ASTNodeKind::GenericTypeDecl: {
            if(linked == current_gen) {
                if (are_types_generic(type->types, current_gen->generic_params)) {
                    // self referential data structure
                    linked_ptr = current_impl_ptr;
                    return;
                }
            }
            // relink generic struct decl with instantiated type
            GenericInstantiator instantiator(container, getAllocator(), diagnoser);
            GenericInstantiatorAPI genApi(&instantiator);
            linked_ptr = linked->as_gen_type_decl_unsafe()->instantiate_type(genApi, type->types);
            return;
        }
        default:
            // we only visit the linked type in this case
            visit(type->referenced);
            break;
    }
}

// suppose user writes T is long <-- we replace T with TypeInsideValue
void replace_is_value_value(IsValue* container, Value* value, ASTAllocator& allocator) {
    switch(value->kind()) {
        case ValueKind::Identifier:
            if(value->kind() == ValueKind::Identifier && value->as_identifier_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam) {
                const auto linked_type = value->as_identifier_unsafe()->linked->as_generic_type_param_unsafe()->known_type();
                const auto replacement = new (allocator.allocate<TypeInsideValue>()) TypeInsideValue(linked_type, value->encoded_location());
                container->value = replacement;
            }
            return;
        case ValueKind::AccessChain:
            if(value->as_access_chain_unsafe()->values.size() == 1) {
                replace_is_value_value(container, value->as_access_chain_unsafe()->values[0], allocator);
            }
        default:
            return;
    }

}

void GenericInstantiator::VisitIsValue(IsValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitIsValue(value);
    replace_is_value_value(value, value->value, getAllocator());
}

void GenericInstantiator::Clear() {
    table.clear();
}

void GenericInstantiator::activateIteration(BaseGenericDecl* gen_decl, size_t itr) {
    auto instantiations = container.getInstantiationTypesFor(gen_decl);
#ifdef DEBUG
    if(itr >= instantiations.size()) {
        throw std::runtime_error("iteration wasn't registered to instantiations container");
    }
#endif
    auto types = instantiations[itr];
    unsigned i = 0;
    for(const auto param : gen_decl->generic_params) {
#ifdef DEBUG
        if(i >= types.size()) {
            throw std::runtime_error("no type for generic parameter exists");
        }
#endif
        param->set_active_type(types[i]);
        i++;
    }
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
    activateIteration(decl, itr);

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

    // replacing parameter types in the function
    for(auto& param : impl->params) {
        visit(param);
    }

    // replace the return type
    visit(impl->returnType);

}

void GenericInstantiator::FinalizeSignature(GenericFuncDecl* decl, FunctionDeclaration* impl, size_t itr) {

    // this allows us to check self-referential pointers to generic decls
    current_gen = decl;
    // set implementation pointer
    current_impl_ptr = impl;

    // activating iteration in params
    activateIteration(decl, itr);

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
    activateIteration(decl, itr);

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
    activateIteration(decl, itr);

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(auto& var : impl->variables()) {
        visit(var);
    }

    // visiting functions
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for (const auto func: impl->functions()) {
        switch(func->kind()) {
            case ASTNodeKind::FunctionDecl:
                table.declare(func->as_function_unsafe()->name_view(), func);
                break;
            case ASTNodeKind::GenericFuncDecl:
                table.declare(func->as_gen_func_decl_unsafe()->name_view(), func);
                break;
            default:
                break;
        }
    }

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(const auto var : impl->variables()) {
        visit(var);
    }

    // visiting functions
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for (const auto func: impl->functions()) {
        switch(func->kind()) {
            case ASTNodeKind::FunctionDecl:
                table.declare(func->as_function_unsafe()->name_view(), func);
                break;
            case ASTNodeKind::GenericFuncDecl:
                table.declare(func->as_gen_func_decl_unsafe()->name_view(), func);
                break;
            default:
                break;
        }
    }

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(const auto var : impl->variables()) {
        visit(var);
    }

    // visiting functions
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for (const auto func: impl->functions()) {
        switch(func->kind()) {
            case ASTNodeKind::FunctionDecl:
                table.declare(func->as_function_unsafe()->name_view(), func);
                break;
            case ASTNodeKind::GenericFuncDecl:
                table.declare(func->as_gen_func_decl_unsafe()->name_view(), func);
                break;
            default:
                break;
        }
    }

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(auto& var : impl->variables()) {
        visit(var);
    }

    // visiting functions
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for (const auto func: impl->functions()) {
        switch(func->kind()) {
            case ASTNodeKind::FunctionDecl:
                table.declare(func->as_function_unsafe()->name_view(), func);
                break;
            case ASTNodeKind::GenericFuncDecl:
                table.declare(func->as_gen_func_decl_unsafe()->name_view(), func);
                break;
            default:
                break;
        }
    }

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // visiting inherited types
    for(auto& inh : impl->inherited) {
        visit(inh.type);
    }

    // visiting variables
    for(auto& var : impl->variables()) {
        visit(var);
    }

    // visiting functions
    for(const auto func : impl->master_functions()) {
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
    activateIteration(decl, itr);

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare function names before visiting
    for (const auto func: impl->functions()) {
        switch(func->kind()) {
            case ASTNodeKind::FunctionDecl:
                table.declare(func->as_function_unsafe()->name_view(), func);
                break;
            case ASTNodeKind::GenericFuncDecl:
                table.declare(func->as_gen_func_decl_unsafe()->name_view(), func);
                break;
            default:
                break;
        }
    }

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto func : impl->master_functions()) {
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
    InstantiationsContainer& container,
    ASTAllocator& astAllocator,
    ASTDiagnoser& diagnoser
) : giPtr(new GenericInstantiator(container, astAllocator, diagnoser)) {

}

GenericInstantiatorAPI::~GenericInstantiatorAPI() {
    if(owns) {
        delete giPtr;
    }
}

InstantiationsContainer& GenericInstantiatorAPI::getContainer() {
    return giPtr->container;
}

ASTAllocator& GenericInstantiatorAPI::getAllocator() {
    return giPtr->getAllocator();
}

ASTDiagnoser& GenericInstantiatorAPI::getDiagnoser() {
    return giPtr->diagnoser;
}

void GenericInstantiatorAPI::setAllocator(ASTAllocator& allocator) {
    giPtr->setAllocator(allocator);
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