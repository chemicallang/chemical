
#include "GenericInstantiator.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "GenInstantiatorAPI.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/values/TypeInsideValue.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "compiler/symres/ImplementationsIndex.h"

void GenericInstantiator::make_gen_type_concrete(BaseType*& type) {
    if(type->kind() == BaseTypeKind::Linked){
        const auto linked = type->as_linked_type_unsafe()->linked;
        switch(linked->kind()) {
            case ASTNodeKind::GenericTypeParam: {
                const auto param = linked->as_generic_type_param_unsafe();
                BaseType* ty = nullptr;
                auto it = active_type_map.find(param);
                if(it != active_type_map.end()) {
                    ty = it->second;
                } else {
                    return;
                }
                if(ty == nullptr) {
                    return;
                }
#ifdef DEBUG
                if(ty == nullptr) {
                    CHEM_THROW_RUNTIME("generic active type doesn't exist");
                }
                if(ty->kind() == BaseTypeKind::Linked && ty->as_linked_type_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam && ty->as_linked_type_unsafe()->linked == linked) {
                    CHEM_THROW_RUNTIME("unexpected generic type parameter usage");
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
    auto instantiator = newGenericInstantiatorFrom(*this);
    GenericInstantiatorAPI genApi(&instantiator);
    // function calls in generic bodies need signature finalization of dependencies
    // (e.g., variant constructors, generic function calls)
    if(!call->instantiate_gen_call(genApi, nullptr, getRequirement())) {
        diagnoser.error("couldn't instantiate call", call);
    }
    // this determines the type for the function call
    call->determine_type(getAllocator(), diagnoser);
}

bool GenericInstantiator::relink_identifier(VariableIdentifier* val) {
    const auto id = val->as_identifier_unsafe();
    const auto node = table.resolve(id->value);
    if(node) {
        id->linked = node;
        switch (node->kind()) {
            case ASTNodeKind::GenericTypeParam: {
                auto found = active_type_map.find(node->as_generic_type_param_unsafe());
                if (found == active_type_map.end()) {
                    return false;
                }
                id->setType(found->second);
            }
            default:
                id->setType(node->getType());
                return true;
        }
    } else {
        return false;
    }
}

void relink_parent(AccessChain* chain, GenericInstantiator& instantiator, FunctionTypeBody* curr_func) {
    const auto values_size = chain->values.size();
    if (values_size <= 1) return;
    unsigned i = 1;
    while (i < values_size) {
        const auto id = chain->values[i]->as_identifier_unsafe();
        const auto parent = chain->values[i - 1];
        auto linked_node = parent->linked_node();
        if(linked_node) {
            const auto child = linked_node->child(&instantiator.child_resolver, id->value);
            if(child) {
                id->linked = child;
                id->setType(child->known_type());
                id->process_linked(&instantiator.diagnoser, curr_func);
            } else {
                id->linked = (ASTNode*) instantiator.typeBuilder.getUnresolvedDecl();
                id->setType(id->linked->known_type());
                instantiator.diagnoser.error(id) << "unresolved child '" << id->value << "' in parent '" << parent->representation() << "'";
                return;
            }
        } else {
            id->linked = (ASTNode*) instantiator.typeBuilder.getUnresolvedDecl();
            id->setType(id->linked->known_type());
            instantiator.diagnoser.error(id) << "unresolved child '" << id->value << "' because parent '" << parent->representation() << "' couldn't be resolved.";
            return;
        }
        i++;
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
            relink_parent(value, *this, current_func_type);
        }
    } else {
        // since it's not a identifier, we must visit it
        visit(val);
        // now we must relink because parent has probably changed type
        relink_parent(value, *this, current_func_type);
    }

    // type can change because of generics, so we keep track of it in every value
    value->setType(value->values.back()->getType());

    // NOTE:
    // we do not need to visit values except the first one in access chain
    // this is because, after the first value, only identifiers are present
    // however we do not need to relink identifiers, if we did visit identifiers
    // it would relink identifiers with any symbols found which would break everything

}

void GenericInstantiator::VisitLinkedType(LinkedType* type) {

    // relink the type if found
    auto type_name = type->linked_name();
    if(!type_name.empty()) {
        const auto node = table.resolve(type_name);
        if (node) {
            type->linked = node;
        }
    }

}

void GenericInstantiator::VisitLambdaFunction(LambdaFunction *func) {

    // the function type of a lambda is inferred from expected type
    // which can be generic, so we must visit that
    VisitFunctionType(func->as_function_type_unsafe());

    // set the current function type to lambda
    const auto prev = current_func_type;
    current_func_type = func;

    // start a scope (we must drop parameters & captured variables symbols, after lambda ends)
    table.scope_start();

    // handle the captured variables
    for(auto& var : func->captureList) {
        // re-resolve the captured variables
        const auto node = table.resolve(var->name);
        if(node) {
            var->linked = node;
        }
        // declare the new captured variable as well
        table.declare(var->name, var);
    }

    // handle the parameters of lambda
    for(const auto param : func->params) {
        table.declare(param->name_view(), param);
    }

    // visit the scope
    visit_it(func->scope);

    // this would drop the parameters and captured variables symbols we introduced
    table.scope_end();

    // set the previous function type
    current_func_type = prev;

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
        auto instantiator = newGenericInstantiatorFrom(*this);
        GenericInstantiatorAPI genApi(&instantiator);
        val->resolve_container(diagnoser);
        // struct values in generic bodies need signature finalization of the specialized container
        val->ensure_specialized_container(genApi, diagnoser, getRequirement());
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
            // explicit nullptr for ChildResolver because we're just looking for direct variant members
            const auto newChild = varDef->child(nullptr, varCase->member->name);
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

void GenericInstantiator::VisitIfValue(IfValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitIfStmt(&value->stmt);
    auto last_val = value->stmt.get_value_node();
    if(last_val) {
        value->setType(last_val->getType());
    }
}

void GenericInstantiator::VisitSwitchValue(SwitchValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitSwitchStmt(&value->stmt);
    const auto node = value->stmt.get_value_node();
    if(node) {
        value->setType(node->getType());
    }
}

void GenericInstantiator::VisitLoopValue(LoopValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitLoopBlock(&value->stmt);
    const auto first = value->stmt.get_first_broken();
    if(first) {
        value->setType(first->getType());
    } else {
        value->setType((BaseType*) typeBuilder.getVoidType());
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

inline SourceLocation gen_type_loc(GenericInstantiator* inst, GenericType* type) {
    if(type->types.empty()) {
        return inst->type_location;
    } else {
        return type->types.front().encoded_location();
    }
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
        case ASTNodeKind::TypealiasStmt: {
            // we inlined this type, now we create concrete type by instantiating it
            const auto alias = linked->as_typealias_unsafe();
            if(alias->attrs.is_inlined) {
                auto instantiator = newGenericInstantiatorFrom(*this);
                GenericInstantiatorAPI genApi(&instantiator);
                // nested generic types encountered during instantiation need signature finalization
                linked_ptr = alias->generic_parent->instantiate_type(genApi, type->types, gen_type_loc(this, type), getRequirement());
            }
            visit(type->referenced);
            return;
        }
        case ASTNodeKind::GenericStructDecl: {
            if (linked == current_gen) {
                if (are_types_generic(type->types, current_gen->generic_params)) {
                    // self referential data structure
                    linked_ptr = current_impl_ptr;
                    return;
                }
            }
            // relink generic struct decl with instantiated type
            auto instantiator = newGenericInstantiatorFrom(*this);
            GenericInstantiatorAPI genApi(&instantiator);
            // nested generic types encountered during instantiation need signature finalization
            linked_ptr = linked->as_gen_struct_def_unsafe()->instantiate_type(genApi, type->types, gen_type_loc(this, type), getRequirement());
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
            auto instantiator = newGenericInstantiatorFrom(*this);
            GenericInstantiatorAPI genApi(&instantiator);
            // nested generic types encountered during instantiation need signature finalization
            linked_ptr = linked->as_gen_union_decl_unsafe()->instantiate_type(genApi, type->types, gen_type_loc(this, type), getRequirement());
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
            auto instantiator = newGenericInstantiatorFrom(*this);
            GenericInstantiatorAPI genApi(&instantiator);
            // nested generic types encountered during instantiation need signature finalization
            linked_ptr = linked->as_gen_interface_decl_unsafe()->instantiate_type(genApi, type->types, gen_type_loc(this, type), getRequirement());
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
            auto instantiator = newGenericInstantiatorFrom(*this);
            GenericInstantiatorAPI genApi(&instantiator);
            // nested generic types encountered during instantiation need signature finalization
            linked_ptr = linked->as_gen_variant_decl_unsafe()->instantiate_type(genApi, type->types, gen_type_loc(this, type), getRequirement());
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
            auto instantiator = newGenericInstantiatorFrom(*this);
            GenericInstantiatorAPI genApi(&instantiator);
            // nested generic types encountered during instantiation need signature finalization
            linked_ptr = linked->as_gen_type_decl_unsafe()->instantiate_type(genApi, type->types, gen_type_loc(this, type), getRequirement());
            return;
        }
        default:
            // we only visit the linked type in this case
            visit(type->referenced);
            break;
    }
}

// suppose user writes T is long <-- we replace T with TypeInsideValue
void replace_is_value_value(GenericInstantiator& inst, IsValue* container, Value* value) {
    switch(value->kind()) {
        case ValueKind::Identifier:
            if(value->kind() == ValueKind::Identifier && value->as_identifier_unsafe()->linked->kind() == ASTNodeKind::GenericTypeParam) {
                const auto param = value->as_identifier_unsafe()->linked->as_generic_type_param_unsafe();
                BaseType* linked_type;
                auto it = inst.active_type_map.find(param);
                if(it != inst.active_type_map.end()) {
                    linked_type = it->second;
                } else {
                    return;
                }
                const auto replacement = new (inst.getAllocator().allocate<TypeInsideValue>()) TypeInsideValue(linked_type, value->encoded_location());
                container->value = replacement;
            }
            return;
        case ValueKind::AccessChain:
            if(value->as_access_chain_unsafe()->values.size() == 1) {
                replace_is_value_value(inst, container, value->as_access_chain_unsafe()->values[0]);
            }
        default:
            return;
    }

}

void GenericInstantiator::VisitIsValue(IsValue* value) {
    replace_is_value_value(*this, value, value->value);
    RecursiveVisitor<GenericInstantiator>::VisitIsValue(value);
}

void GenericInstantiator::VisitComptimeValue(ComptimeValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitComptimeValue(value);
    // type can change due to generics, so we must redetermine it
    value->setType(value->getValue()->getType());
}

void GenericInstantiator::VisitIncDecValue(IncDecValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitIncDecValue(value);
    value->getValue()->report_assignment_of_chain_id();
    // type can change due to generics
    value->setType(value->determine_type(diagnoser, coreNodes, implsIndex));
}

void GenericInstantiator::VisitAssignmentStmt(AssignStatement *assign) {
    RecursiveVisitor<GenericInstantiator>::VisitAssignmentStmt(assign);
    assign->lhs->report_assignment_of_chain_id();
}

void GenericInstantiator::VisitAddrOfValue(AddrOfValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitAddrOfValue(value);
    value->value->report_addr_taken_of_chain_id();
    // type can change, must redetermine it
    value->determine_type();
}

void GenericInstantiator::VisitReferenceOfValue(ReferenceOfValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitReferenceOfValue(value);
    value->value->report_addr_taken_of_chain_id();
    // type can change, must redetermine it
    value->determine_type();
}

void GenericInstantiator::VisitDereferenceValue(DereferenceValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitDereferenceValue(value);
    if(!value->determine_type(typeBuilder)) {
        diagnoser.error("only pointer types can be de-referenced", value);
    }
}

void GenericInstantiator::VisitRuntimeValue(RuntimeValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitRuntimeValue(value);
    value->getType()->as_runtime_type_unsafe()->underlying = value->underlying->getType();
}

void GenericInstantiator::VisitExpression(Expression *expr) {
    RecursiveVisitor<GenericInstantiator>::VisitExpression(expr);
    expr->determine_type(typeBuilder, coreNodes, implsIndex, diagnoser, targetData);
}

void GenericInstantiator::VisitIndexOperator(IndexOperator* value) {
    RecursiveVisitor<GenericInstantiator>::VisitIndexOperator(value);
    value->determine_type(typeBuilder, coreNodes, implsIndex, diagnoser);
}

void GenericInstantiator::VisitNegativeValue(NegativeValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitNegativeValue(value);
    value->determine_type(typeBuilder, coreNodes, implsIndex, diagnoser);
}

void GenericInstantiator::VisitUnsafeValue(UnsafeValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitUnsafeValue(value);
    // re-determine type for child value
    value->setType(value->getValue()->getType());
}

void GenericInstantiator::VisitNewValue(NewValue *value) {
    RecursiveVisitor<GenericInstantiator>::VisitNewValue(value);
    value->ptr_type.type = value->value->getType();
}

void GenericInstantiator::VisitPlacementNewValue(PlacementNewValue *value) {
    RecursiveVisitor<GenericInstantiator>::VisitPlacementNewValue(value);
    value->ptr_type.type = value->value->getType();
}

void GenericInstantiator::VisitNotValue(NotValue* value) {
    RecursiveVisitor<GenericInstantiator>::VisitNotValue(value);
    value->determine_type(diagnoser, coreNodes, implsIndex);
}

void GenericInstantiator::VisitBitwiseNot(BitwiseNot* value) {
    RecursiveVisitor<GenericInstantiator>::VisitBitwiseNot(value);
    value->determine_type(diagnoser, coreNodes, implsIndex);
}

static bool embedded_traverse(void* data, ASTAny* item) {
    const auto visitor = static_cast<GenericInstantiator*>(data);
    switch(item->any_kind()) {
        case ASTAnyKind::Value:
            visitor->VisitValueNoNullCheck(static_cast<Value*>(item));
            return true;
        case ASTAnyKind::Type:
            visitor->VisitTypeNoNullCheck(static_cast<BaseType*>(item));
            return true;
        case ASTAnyKind::Node:
            visitor->VisitNodeNoNullCheck(static_cast<ASTNode*>(item));
            return true;
        default:
            return false;
    }
}

void GenericInstantiator::Clear() {
    table.clear();
    active_type_map.clear();
}

void GenericInstantiator::activateIteration(BaseGenericDecl* gen_decl, size_t itr) {
    // lock to prevent concurrent modification of the container's internal
    // hash map / vectors while we read the instantiation types
    std::lock_guard<std::recursive_mutex> lock(registration_mutex);
    auto instantiations = container.getInstantiationTypesFor(gen_decl);
#ifdef DEBUG
    if(itr >= instantiations.size()) {
        CHEM_THROW_RUNTIME("iteration wasn't registered to instantiations container");
    }
#endif
    auto types = instantiations[itr];
    unsigned i = 0;
    for(const auto param : gen_decl->generic_params) {
#ifdef DEBUG
        if(i >= types.size()) {
            CHEM_THROW_RUNTIME("no type for generic parameter exists");
        }
#endif
        const auto t = types[i];
        switch(t->kind()) {
            case BaseTypeKind::Linked:
                table.declare(param->identifier, t->as_linked_type_unsafe()->linked);
                break;
            case BaseTypeKind::Generic:
                table.declare(param->identifier, t->as_generic_type_unsafe()->referenced->linked);
                break;
            default:
                break;
        }
        active_type_map[param] = t;
        i++;
    }
}

void GenericInstantiator::waitSignatureFinalized(BaseGenericDecl* decl, size_t index) {
    auto& status_mutex = container.getInstantiationStatusMutex();
    auto& cv = container.getInstantiationCv();
    std::unique_lock<std::mutex> lock(status_mutex);
    auto& entry = decl->instantiation_statuses[index];
    cv.wait(lock, [&entry]() {
        return entry.status == InstantiationStatus::SignatureFinalized;
    });
}

void GenericInstantiator::notifySignatureFinalized(BaseGenericDecl* decl, size_t index) {
    auto& status_mutex = container.getInstantiationStatusMutex();
    auto& cv = container.getInstantiationCv();
    {
        std::lock_guard<std::mutex> lock(status_mutex);
        decl->instantiation_statuses[index].status = InstantiationStatus::SignatureFinalized;
    }
    cv.notify_all();
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

void GenericInstantiator::FinalizeSignature(GenericTypeDecl* decl, TypealiasStatement* impl, std::vector<TypeLoc>& generic_args) {

    // this allows us to check self-referential pointers to generic decls
    current_gen = decl;

    // activating iteration in params
    unsigned i = 0;
    for(const auto param : decl->generic_params) {
        active_type_map[param] = generic_args[i];
        i++;
    }

    // finalize the signature of typealias instantiation
    FinalizeSignature(impl);

    // deactivating iteration in parameters
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
    }

    // reset the pointers
    current_gen = nullptr;
    current_impl_ptr = nullptr;

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
        active_type_map.erase(param);
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
        active_type_map.erase(param);
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
        current_func_type = impl;
        visit(impl->body.value());

        // end the body scope
        table.scope_end();
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
    }

    // reset the pointers
    current_gen = nullptr;
    current_impl_ptr = nullptr;


}

void handle_new_impl(GenericInstantiator& instantiator, ImplDefinition* node, MembersContainer* adopter) {
    auto& diagnoser = instantiator.diagnoser;
    adopter->adopt(node);
    const auto linked_node = node->interface_type->get_direct_linked_node();
    if (linked_node->kind() == ASTNodeKind::InterfaceDecl) {
        auto& implsIndex = instantiator.implsIndex;
        const auto linked = linked_node->as_interface_def_unsafe();
        if (linked->is_static() && linked->has_implementation()) {
            diagnoser.error("static interface must have only a single implementation", node->encoded_location());
        }
        linked->register_impl(node);
        implsIndex.add_interface(linked, adopter, node);
    } else {
        diagnoser.error("expected type to be an interface", node->interface_type.encoded_location());
    }
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
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                FinalizeSignature(node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                FinalizeSignature(node->as_gen_func_decl_unsafe()->master_impl);
                break;
            case ASTNodeKind::ImplDecl: {
                const auto def = node->as_impl_def_unsafe();
                FinalizeNestedImplSignature(def);
                handle_new_impl(*this, def, impl);
                continue;
            }
            default:
                visit(node);
        }
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;

}

void finalize_member_func_body(GenericInstantiator& instantiator, FunctionDeclaration* func) {
    if(func->body.has_value()) {
        // get the table
        auto& table = instantiator.table;

        // start scope for function body
        table.scope_start();

        // declare function parameters
        for(const auto param : func->params) {
            table.declare(param->name_view(), param);
        }

        // visit the body
        instantiator.current_func_type = func;
        instantiator.visit(func->body.value());

        // end the body scope
        table.scope_end();
    }
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

    // declare functions / (other declarable nodes) before visiting
    for (const auto func: impl->evaluated_nodes()) {
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
    auto& instantiator  = *this;
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                finalize_member_func_body(instantiator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                finalize_member_func_body(instantiator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            case ASTNodeKind::ImplDecl: {
                const auto def = node->as_impl_def_unsafe();
                FinalizeNestedImplBody(def);
                continue;
            }
            default:
                visit(node);
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
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
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                FinalizeSignature(node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                FinalizeSignature(node->as_gen_func_decl_unsafe()->master_impl);
                break;
            case ASTNodeKind::ImplDecl: {
                const auto def = node->as_impl_def_unsafe();
                FinalizeNestedImplSignature(def);
                handle_new_impl(*this, def, impl);
                continue;
            }
            default:
                visit(node);
        }
    }

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
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

    // declare functions / (other declarable nodes) before visiting
    for (const auto func: impl->evaluated_nodes()) {
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
    auto& instantiator  = *this;
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                finalize_member_func_body(instantiator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                finalize_member_func_body(instantiator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            case ASTNodeKind::ImplDecl: {
                const auto def = node->as_impl_def_unsafe();
                FinalizeNestedImplBody(def);
                continue;
            }
            default:
                visit(node);
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
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
        active_type_map.erase(param);
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

    // declare functions / (other declarable nodes) before visiting
    for (const auto func: impl->evaluated_nodes()) {
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
    auto& instantiator  = *this;
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                finalize_member_func_body(instantiator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                finalize_member_func_body(instantiator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            default:
                visit(node);
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
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
        active_type_map.erase(param);
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

    // declare functions / (other declarable nodes) before visiting
    for (const auto func: impl->evaluated_nodes()) {
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
    auto& instantiator  = *this;
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                finalize_member_func_body(instantiator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                finalize_member_func_body(instantiator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            case ASTNodeKind::ImplDecl: {
                const auto def = node->as_impl_def_unsafe();
                FinalizeNestedImplBody(def);
                continue;
            }
            default:
                visit(node);
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;
}

void GenericInstantiator::FinalizeNestedImplSignature(ImplDefinition* impl) {

    // ----
    // lets do signature first
    // ----

    visit_it(impl->interface_type);
    visit_it(impl->struct_type);

    // visiting variables
    for(auto& var : impl->variables()) {
        visit(var);
    }

    // visiting function bodies (only bodies, because we finalized signature above)
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                FinalizeSignature(node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                FinalizeSignature(node->as_gen_func_decl_unsafe()->master_impl);
                break;
            default:
                visit(node);
        }
    }

}

void GenericInstantiator::FinalizeNestedImplBody(ImplDefinition* impl) {

    // create a symbol scope
    table.scope_start();

    // declare variables
    for(const auto var : impl->variables()) {
        table.declare(var->name, var);
    }

    // declare functions / (other declarable nodes) before visiting
    for (const auto func: impl->evaluated_nodes()) {
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
    auto& instantiator  = *this;
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                finalize_member_func_body(instantiator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                finalize_member_func_body(instantiator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            default:
                visit(node);
        }
    }

    // end the symbol scope
    table.scope_end();

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
        active_type_map.erase(param);
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

    // declare functions / (other declarable nodes) before visiting
    for (const auto func: impl->evaluated_nodes()) {
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
    auto& instantiator  = *this;
    for(const auto node : impl->evaluated_nodes()) {
        switch (node->kind()) {
            case ASTNodeKind::FunctionDecl:
                finalize_member_func_body(instantiator, node->as_function_unsafe());
                break;
            case ASTNodeKind::GenericFuncDecl:
                finalize_member_func_body(instantiator, node->as_gen_func_decl_unsafe()->master_impl);
                break;
            default:
                visit(node);
        }
    }

    // end the symbol scope
    table.scope_end();

    // deactivating iteration in parameters
    // activating iteration in params
    for(const auto param : decl->generic_params) {
        active_type_map.erase(param);
    }

    // reset back the pointers to null
    current_gen = nullptr;
    current_impl_ptr = nullptr;
}

void GenericInstantiator::FinalizeSignature(GenericTypeDecl* decl, const std::span<TypealiasStatement*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
        Clear();
    }
}

void GenericInstantiator::FinalizeSignature(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations) {
    for(const auto inst : instantiations) {
        FinalizeSignature(decl, inst, inst->generic_instantiation);
        Clear();
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
        Clear();
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
        Clear();
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
        Clear();
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
        Clear();
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
        Clear();
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
    AnnotationController& controller,
    CompilerBinder& binder,
    ChildResolver& child_resolver,
    InstantiationsContainer& container,
    CoreNodes& coreNodes,
    ImplementationsIndex& implsIndex,
    std::recursive_mutex& registration_mutex,
    ASTAllocator& astAllocator,
    ASTDiagnoser& diagnoser,
    TypeBuilder& typeBuilder,
    TargetData& targetData
) : giPtr(new GenericInstantiator(controller, binder, child_resolver, container, coreNodes, implsIndex, registration_mutex, astAllocator, diagnoser, typeBuilder, targetData)) {

}

GenericInstantiatorAPI GenericInstantiatorAPI::newGenericInstantiatorFrom(const GenericInstantiatorAPI& other) {
    auto& g = *other.giPtr;
    return GenericInstantiatorAPI(
        g.controller, g.binder, g.child_resolver,
        g.container, g.coreNodes, g.implsIndex, g.registration_mutex,
        *g.allocator_ptr, g.diagnoser, g.typeBuilder, g.targetData
    );
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

std::recursive_mutex& GenericInstantiatorAPI::getRegistrationMutex() {
    return giPtr->registration_mutex;
}

std::mutex& GenericInstantiatorAPI::getInstantiationStatusMutex() {
    return giPtr->container.getInstantiationStatusMutex();
}

std::condition_variable& GenericInstantiatorAPI::getInstantiationCv() {
    return giPtr->container.getInstantiationCv();
}

void GenericInstantiatorAPI::waitSignatureFinalized(BaseGenericDecl* decl, size_t index) {
    giPtr->waitSignatureFinalized(decl, index);
}

void GenericInstantiatorAPI::notifySignatureFinalized(BaseGenericDecl* decl, size_t index) {
    giPtr->notifySignatureFinalized(decl, index);
}

void GenericInstantiatorAPI::setAllocator(ASTAllocator& allocator) {
    giPtr->setAllocator(allocator);
}

// API Functions

void GenericInstantiatorAPI::FinalizeSignature(GenericTypeDecl* decl, TypealiasStatement* impl, std::vector<TypeLoc>& generic_args) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, impl, generic_args);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeSignature(GenericTypeDecl* decl, const std::span<TypealiasStatement*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeSignature(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeBody(GenericFuncDecl* decl, const std::span<FunctionDeclaration*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::SignatureFinalization);
    g.FinalizeBody(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeSignature(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeBody(GenericStructDecl* decl, const std::span<StructDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::SignatureFinalization);
    g.FinalizeBody(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeSignature(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeBody(GenericUnionDecl* decl, const std::span<UnionDef*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::SignatureFinalization);
    g.FinalizeBody(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeSignature(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeBody(GenericInterfaceDecl* decl, const std::span<InterfaceDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::SignatureFinalization);
    g.FinalizeBody(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeSignature(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeBody(GenericVariantDecl* decl, const std::span<VariantDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::SignatureFinalization);
    g.FinalizeBody(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeSignature(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::Registration);
    g.FinalizeSignature(decl, instantiations);
    g.debug_unsetRequirement();
}

void GenericInstantiatorAPI::FinalizeBody(GenericImplDecl* decl, const std::span<ImplDefinition*>& instantiations) {
    auto& g = *giPtr;
    g.setRequirement(InstantiationRequirement::SignatureFinalization);
    g.FinalizeBody(decl, instantiations);
    g.debug_unsetRequirement();
}