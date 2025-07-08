// Copyright (c) Chemical Language Foundation 2025.

#include "ast/statements/Assignment.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/Break.h"
#include "ast/statements/DestructStmt.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/statements/Return.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/ComptimeBlock.h"
#include "ast/structures/EnumDeclaration.h"
#include "ast/structures/EnumMember.h"
#include "ast/structures/FunctionParam.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/CapturedVariable.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/If.h"
#include "ast/structures/LoopBlock.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/InitBlock.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/values/VariantCase.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/structures/VariantMemberParam.h"
#include "ast/structures/TryCatch.h"
#include "ast/values/ValueNode.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/FunctionCall.h"
#include "ast/types/ArrayType.h"
#include "compiler/symres/SymbolResolver.h"
#include "DeclareTopLevel.h"
#include "ast/utils/ASTUtils.h"

void AssignStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(lhs->link_assign(linker, lhs, nullptr)) {
        BaseType* lhsType = lhs->create_type(linker.allocator);
        if(value->link(linker, value, lhsType)) {
            switch(assOp){
                case Operation::Assignment:
                    if (!lhsType->satisfies(linker.allocator, value, true)) {
                        linker.unsatisfied_type_err(value, lhsType);
                    }
                    break;
                case Operation::Addition:
                case Operation::Subtraction:
                    if(lhsType->kind() == BaseTypeKind::Pointer) {
                        const auto rhsType = value->create_type(linker.allocator)->canonical();
                        if(rhsType->kind() != BaseTypeKind::IntN) {
                            linker.unsatisfied_type_err(value, lhsType);
                        }
                    } else if (!lhsType->satisfies(linker.allocator, value, true)) {
                        linker.unsatisfied_type_err(value, lhsType);
                    }
                    break;
                default:
                    break;
            }
        }
        auto id = lhs->as_identifier();
        if(id) {
            auto linked = id->linked_node();
            auto linked_kind = linked->kind();
            if(linked_kind == ASTNodeKind::VarInitStmt) {
                auto init = linked->as_var_init_unsafe();
                init->set_has_assignment();
            } else if(linked_kind == ASTNodeKind::FunctionParam) {
                auto param = linked->as_func_param_unsafe();
                param->set_has_assignment();
            }
        }
        if(!lhs->check_is_mutable(linker.allocator, true)) {
            linker.error("cannot assign to a non mutable value", lhs);
        }
        auto& func_type = *linker.current_func_type;
        func_type.mark_moved_value(linker.allocator, value, lhs->known_type(), linker, true);
        func_type.mark_un_moved_lhs_value(lhs, lhs->known_type());
    }
}

void UsingStmt::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    if(!is_failed_chain_link()) {
        // we need to declare symbols once again, because all files in a module link signature
        // and then declare_and_link of all files is called, so after link_signature of each
        // file, symbols are dropped
        declare_symbols(linker);
    }
}

void BreakStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(value) {
        value->link(linker, value);
    }
}

void DestructStmt::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(array_value) {
        array_value->link(linker, array_value);
    }
    if(!identifier->link(linker, identifier)) {
        return;
    }
    auto type = identifier->get_canonical_type(linker.allocator);
    if(!type->is_pointer()) {
        linker.error("destruct cannot be called on a value that isn't a pointer", this);
        return;
    }
    auto found = linker.find("free");
    if(!found || !found->as_function()) {
        linker.error("'free' function should be declared before using destruct so calls can be made to it", this);
        return;
    }
    free_func_linked = found->as_function();
}

void ProvideStmt::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(value->link(linker, value, nullptr)) {
        put_in(linker.implicit_args, value, &linker, [](ProvideStmt* stmt, void* data) {
            stmt->body.link_sequentially((*(SymbolResolver*) data));
        });
    }
}

void ReturnStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if (value) {
        const auto func_type = linker.current_func_type;
        if(!value->link(linker, value, func_type->returnType ? func_type->returnType : nullptr)) {
            return;
        }
        if(func_type->data.signature_resolved && func_type->returnType) {
            const auto func = func_type->as_function();
            if(func && func->is_constructor_fn()) {
                return;
            }
            const auto implicit = func_type->returnType->implicit_constructor_for(linker.allocator, value);
            if (implicit &&
                // this check means current function is not the implicit constructor we're trying to link for value
                // basically an implicit constructor can has a value returned of a type for which it's an implicit constructor of (in comptime)
                implicit != func_type &&
                // this check means current function's parent (if it's inside a struct) is not the same parent as the implicit constructor parent
                // meaning implicit constructor and the function that's going to use the implicit constructor can't be inside same container
                (func && func->parent() != implicit->parent())
                    ) {
                link_with_implicit_constructor(implicit, linker, value);
                return;
            }
            if(!func_type->returnType->satisfies(linker.allocator, value, false)) {
                linker.unsatisfied_type_err(value, func_type->returnType);
            }
        }
    }
}

VariantCase* create_variant_case(SymbolResolver& resolver, SwitchStatement* stmt, VariantDefinition* def, VariableIdentifier* id) {
    auto& allocator = *resolver.ast_allocator;
    const auto child = def->child(id->value);
    if(child) {
        if(child->kind() == ASTNodeKind::VariantMember) {
            return new (allocator.allocate<VariantCase>()) VariantCase(child->as_variant_member_unsafe(), stmt, id->encoded_location());
        } else {
            resolver.error(id) << "couldn't find variant member with name '" << id->value << "'";
        }
    } else {
        resolver.error(id) << "couldn't find the variant member with name '" << id->value << "'";
    }
    return nullptr;
}

VariantCase* create_variant_case(SymbolResolver& resolver, SwitchStatement* stmt, VariantDefinition* def, FunctionCall* call) {
    auto& astAlloc = *resolver.ast_allocator;
    if(call->parent_val->kind() == ValueKind::Identifier) {
        const auto first_id = call->parent_val->as_identifier_unsafe();
        const auto varCase = create_variant_case(resolver, stmt, def, first_id);
        if(varCase) {
            // put all values as variant case variables
            for (const auto value: call->values) {
                if (value->kind() == ValueKind::Identifier) {
                    const auto id = value->as_identifier_unsafe();
                    const auto param = varCase->member->values.find(id->value);
                    if (param != varCase->member->values.end()) {

                        auto variable = new(astAlloc.allocate<VariantCaseVariable>()) VariantCaseVariable(id->value, param->second, stmt, 0);
                        varCase->identifier_list.emplace_back(variable);
                        variable->declare_and_link(resolver, (ASTNode*&) (variable));

                    } else {
                        resolver.error("couldn't find variant member parameter with that name", value);
                    }
                } else {
                    resolver.error("expected value to be a identifier", value);
                }
            }
            return varCase;
        }
        return nullptr;
    } else {
        resolver.error("expected first value in the function call to be identifier", call->parent_val);
    }
    return nullptr;
}

bool SwitchStatement::declare_and_link(SymbolResolver &linker, Value** value_ptr) {
    VariantDefinition* variant_def = nullptr;
    bool result = true;
    if(expression->link(linker, expression)) {
        variant_def = getVarDefFromExpr();
        if (value_ptr && variant_def && (scopes.size() < variant_def->variables().size() && !has_default_case())) {
            linker.error("expected all cases of variant in switch statement when no default case is specified", (ASTNode*) this);
            return false;
        }
    } else {
        result = false;
    }

    if(result && value_ptr) {
        auto val_node = get_value_node();
        if(!val_node) {
            linker.error("expected a single value node for the value", (ASTNode*) this);
            return false;
        }
    }

    std::vector<VariableIdentifier*> moved_ids;
    std::vector<AccessChain*> moved_chains;

    unsigned i = 0;
    const auto scopes_size = scopes.size();
    while(i < scopes_size) {
        auto& scope = scopes[i];
        linker.scope_start();
        for(auto& switch_case : cases) {
            if(switch_case.second == i && switch_case.first) {
                if(variant_def) {
                    // it's a variant definition, declare the identifier list
                    // link with the case
                    const auto case_kind = switch_case.first->val_kind();
                    switch(case_kind) {
                        case ValueKind::Identifier: {
                            const auto varCase = create_variant_case(linker, this, variant_def, switch_case.first->as_identifier_unsafe());
                            if (varCase) {
                                switch_case.first = varCase;
                            }
                            continue;
                        }
                        case ValueKind::FunctionCall: {
                            const auto varCase = create_variant_case(linker, this, variant_def, switch_case.first->as_func_call_unsafe());
                            if (varCase) {
                                switch_case.first = varCase;
                            }
                            continue;
                        }
                        case ValueKind::AccessChain:{
                            const auto chain = switch_case.first->as_access_chain_unsafe();
                            if(chain && chain->values.size() == 1) {
                                const auto kind = chain->values.back()->val_kind();
                                if(kind == ValueKind::FunctionCall) {
                                    const auto varCase = create_variant_case(linker, this, variant_def, chain->values.back()->as_func_call_unsafe());
                                    if(varCase) {
                                        switch_case.first = varCase;
                                    }
                                } else if(kind == ValueKind::Identifier) {
                                    const auto varCase = create_variant_case(linker, this, variant_def, chain->values.back()->as_identifier_unsafe());
                                    if(varCase) {
                                        switch_case.first = varCase;
                                    }
                                } else {
                                    linker.error("unknown value in switch when resolving variant cases", switch_case.first);
                                }
                            } else {
                                linker.error("unknown value in switch when resolving variant cases", switch_case.first);
                            }
                            continue;
                        }
                        default:
                            linker.error("unknown value in switch when resolving variant cases", switch_case.first);
                    }
                } else {
                    // link the switch case value
                    switch_case.first->link(linker, switch_case.first);
                }
            }
        }
        linker.link_body_seq_backing_moves(scope, moved_ids, moved_chains);
        linker.scope_end();
        i++;
    }

    // restoring all the moved identifiers and chains, in all the scopes
    const auto curr_func = linker.current_func_type;
    if(curr_func) {
        curr_func->restore_moved_ids(moved_ids);
        curr_func->restore_moved_chains(moved_chains);
    }

    return result;
}

void TypealiasStatement::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    if(!is_top_level()) {
        linker.declare_node(name_view(), this, specifier(), false);
        actual_type.link(linker);
    }
}

void VarInitStatement::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(is_top_level()) {
        if(attrs.signature_resolved && !type && value) {
            type = {value->create_type(*linker.ast_allocator), type.getLocation()};
        }
    } else {
        const auto type_resolved = !type || type.link(linker);
        const auto value_resolved = !value || value->link(linker, value, type_ptr_fast());
        if (!type_resolved || !value_resolved) {
            attrs.signature_resolved = false;
        }
        linker.declare(id_view(), this);
        if (attrs.signature_resolved) {
            if(value) {
                linker.current_func_type->mark_moved_value(linker.allocator, value, known_type(), linker, type != nullptr);
            }
            if(type && value) {
                const auto as_array = value->as_array_value();
                if(type->kind() == BaseTypeKind::Array && as_array) {
                    const auto arr_type = ((ArrayType*) type.getType());
                    if(arr_type->has_no_array_size()) {
                        arr_type->set_array_size(as_array->array_size());
                    } else if(!as_array->has_explicit_size()) {
                        as_array->set_array_size(arr_type->get_array_size());
                    }
                }
                if(!type->satisfies(linker.allocator, value, false)) {
                    linker.unsatisfied_type_err(value, type);
                }
            }
            if(!type && value && !linker.generic_context) {
                type = {value->create_type(*linker.ast_allocator), type.getLocation()};
            }
        }
    }
}

void ComptimeBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    body.link_sequentially(linker);
}

void DoWhileLoop::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    body.link_sequentially(linker);
    condition->link(linker, condition);
    linker.scope_end();
}

void EnumMember::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    if(init_value) {
        init_value->link(linker, init_value, nullptr);
    }
    linker.declare(name, this);
}

void configure_members_by_inheritance(EnumDeclaration* current, int start) {
    // build sorted list of enum members (sorted by the index specified by the user)
    std::vector<EnumMember*> sorted_members;
    sorted_members.reserve(current->members.size());
    for(auto& member : current->members) {
        sorted_members.emplace_back(member.second);
    }
    std::stable_sort(std::begin(sorted_members), std::end(sorted_members), [](EnumMember* a, EnumMember* b) {
        return a->get_index_dirty() < b->get_index_dirty();
    });
    // now that we have a sorted list of members for current enum
    // we should modify its member index as long as we can predict it
    auto counter = 0;
    for(const auto mem : sorted_members) {
        if(mem->get_index_dirty() == counter) {
            // this means user didn't modify the index
            // we should modify it
            mem->set_index_dirty(start);
            start++;
            counter++;
        } else {
            return;
        }
    }
}

void EnumDeclaration::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    const auto pure_underlying = underlying_type->pure_type(*linker.ast_allocator);
    const auto k = pure_underlying->kind();
    if(k == BaseTypeKind::IntN) {
        underlying_integer_type = pure_underlying->as_intn_type_unsafe();
    } else {
        const auto linked = pure_underlying->get_direct_linked_node();
        if(linked->kind() == ASTNodeKind::EnumDecl) {
            const auto inherited = linked->as_enum_decl_unsafe();
            configure_members_by_inheritance(this, inherited->next_start);
            underlying_integer_type = inherited->underlying_integer_type;
        } else {
            linker.error("given type is not an enum or integer type", encoded_location());
            underlying_integer_type = new (linker.ast_allocator->allocate<IntType>()) IntType();
        }
    }
    linker.scope_start();
    // since members is an unordered map, first we declare all enums
    // then we link their init values
    for(auto& mem : members) {
        linker.declare(mem.first, mem.second);
    }
    // since now all identifiers will be available regardless of order of the map
    for(auto& mem : members) {
        auto& value = mem.second->init_value;
        if(value) {
            value->link(linker, value, nullptr);
        }
    }
    linker.scope_end();
}

void ForLoop::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    initializer->declare_and_link(linker, (ASTNode*&) initializer);
    conditionExpr->link(linker, conditionExpr);
    incrementerExpr->declare_and_link(linker, incrementerExpr);
    body.link_sequentially(linker);
    linker.scope_end();
}

void FunctionParam::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.declare(name, this);
}

void GenericTypeParameter::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(at_least_type) {
        at_least_type.link(linker);
    }
    declare_only(linker);
    if(def_type) {
        def_type.link(linker);
    }
}

void FunctionDeclaration::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(body.has_value()) {
        // if has body declare params
        linker.scope_start();
        auto prev_func_type = linker.current_func_type;
        linker.current_func_type = this;
        for (auto& param : params) {
            param->declare_and_link(linker, (ASTNode*&) param);
        }
        if(FunctionType::data.signature_resolved) {
            if(is_comptime()) {
                linker.comptime_context = true;
            }
            body->link_sequentially(linker);
            linker.comptime_context = false;
        }
        linker.scope_end();
        linker.current_func_type = prev_func_type;
    }

}

bool CapturedVariable::declare_and_link(SymbolResolver& linker) {
    const auto found = linker.find(name);
    const auto has = found != nullptr;
    if(has) {
        linked = found;
    }
    linker.declare(name, this);
    return has;
}

void GenericFuncDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    // symbol resolve the master declaration
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    linker.generic_context = prev_gen_context;
    linker.scope_end();
    body_linked = true;
    // finalizing the body of every function that was instantiated before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : instantiations) {
        finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(this, instantiations);
}

void GenericImplDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    linker.generic_context = prev_gen_context;
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

void GenericInterfaceDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    master_impl->declare_and_link_no_scope(linker);
    linker.generic_context = prev_gen_context;
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

void GenericStructDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    linker.generic_context = prev_gen_context;
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

void GenericUnionDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    linker.generic_context = prev_gen_context;
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

void GenericVariantDecl::declare_and_link(SymbolResolver &linker, ASTNode *&node_ptr) {
    linker.scope_start();
    for(auto& param : generic_params) {
        param->declare_and_link(linker, (ASTNode*&) param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    master_impl->declare_and_link(linker, (ASTNode*&) master_impl);
    linker.generic_context = prev_gen_context;
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

void link_body(
        Scope& body,
        Value*& conditionExpr,
        SymbolResolver& linker,
        std::vector<VariableIdentifier*>& moved_ids,
        std::vector<AccessChain*>& moved_chains
) {
    linker.scope_start();
    if(conditionExpr->kind() == ValueKind::PatternMatchExpr) {
        // we must link it here
        // since pattern matching introduces symbols to link against
        conditionExpr->link(linker, conditionExpr, nullptr);
    }
    linker.link_body_seq_backing_moves(body, moved_ids, moved_chains);
    linker.scope_end();
}

void IfStatement::declare_and_link(SymbolResolver &linker, Value** value_ptr) {

    if(is_top_level()) {
        auto scope = get_evaluated_scope_by_linking(linker);
        if(scope) {
            scope->declare_and_link(linker);
        }
        return;
    }

    link_conditions_no_patt_match_expr(linker);

    if(is_computable || compile_time_computable()) {
        is_computable = true;
        if(computed_scope.has_value()) {
            auto scope = computed_scope.value();
            if(scope) {
                scope->declare_and_link(linker, (ASTNode*&) computed_scope.value());
            }
            return;
        }
        if(!linker.comptime_context) {
            auto condition_val = resolved_condition ? get_condition_const((InterpretScope&) linker.comptime_scope) : std::optional(false);
            if (condition_val.has_value()) {
                auto eval = get_evaluated_scope((InterpretScope&) linker.comptime_scope, &linker, condition_val.value());
                computed_scope = eval;
                if (eval) {
                    eval->declare_and_link(linker, (ASTNode*&) computed_scope.value());
                }
                return;
            } else {
                is_computable = false;
            }
        }
    }

    // current func type is only not present when its a top level if
    // or maybe during parsing malformed code can cause it
    const auto curr_func = linker.current_func_type;
    if(!curr_func) return;

    // temporary moved identifiers and chains
    std::vector<VariableIdentifier*> moved_ids;
    std::vector<AccessChain*> moved_chains;

    // link the body
    link_body(ifBody, condition, linker, moved_ids, moved_chains);
    // link the else ifs
    for(auto& elseIf : elseIfs) {
        link_body(elseIf.second, elseIf.first, linker, moved_ids, moved_chains);
    }
    // link the else body
    if(elseBody.has_value()) {
        linker.scope_start();
        linker.link_body_seq_backing_moves(elseBody.value(), moved_ids, moved_chains);
        linker.scope_end();
    }

    curr_func->restore_moved_ids(moved_ids);
    curr_func->restore_moved_chains(moved_chains);
}

void ImplDefinition::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    const auto linked_node = interface_type->linked_node();
    if(!linked_node) {
        return;
    }
    const auto linked = linked_node->as_interface_def();
    if(!linked) {
        return;
    }
    linker.scope_start();
    const auto struct_linked = struct_type ? struct_type->linked_struct_def() : nullptr;
    const auto overrides_interface = struct_linked && struct_linked->does_override(linked);
    if(!overrides_interface) {
        for (const auto func: linked->functions()) {
            switch(func->kind()) {
                case ASTNodeKind::FunctionDecl:
                    linker.declare(func->as_function_unsafe()->name_view(), func);
                    break;
                case ASTNodeKind::GenericFuncDecl:
                    linker.declare(func->as_gen_func_decl_unsafe()->name_view(), func);
                    break;
                default:
                    break;
            }
        }
    }
    // redeclare everything inside struct
    if(struct_linked) {
        struct_linked->redeclare_inherited_members(linker);
        struct_linked->redeclare_variables_and_functions(linker);
    }
    MembersContainer::declare_and_link_no_scope(linker);
    linker.scope_end();
}

void Namespace::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    if(root) {
        root->declare_extended_in_linker(linker);
    } else {
        TopLevelDeclSymDeclare declarer(linker);
        for(auto& node : nodes) {
            declarer.visit(node);
        }
    }
    for(auto& node : nodes) {
        node->declare_and_link(linker, node);
    }
    linker.scope_end();
}

void Scope::declare_and_link(SymbolResolver &linker) {
    for (auto &node: nodes) {
        node->declare_and_link(linker, node);
    }
}

void LoopBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    body.link_sequentially(linker);
}

void InitBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto mems_container = getContainer();
    if(!mems_container) {
        linker.error("unexpected init block", this);
        return;
    }
    for(auto& in : initializers) {
        in.second.value->link(linker, in.second.value, nullptr);
    }
//    // now taking out initializers
//    for(const auto node : scope.nodes) {
//        const auto val_wrapper = node->as_value_wrapper();
//        if(!val_wrapper) {
//            linker.error("expected members of init block to be initializer call", (ASTNode*) node);
//            continue;
//        }
//        auto chain = val_wrapper->value->as_access_chain();
//        if(!chain) {
//            linker.error("expected members of init block to be initializer call", (ASTNode*) node);
//            continue;
//        }
//        auto& call_ptr = chain->values.back();
//        auto call = call_ptr->as_func_call();
//        if(!call) {
//            linker.error("expected members of init block to be initializer call", (ASTNode*) chain);
//            continue;
//        }
////        const auto chain_size = chain->values.size();
////        if(chain_size < 2) {
////            linker.error("expected members of init block to be initializer call", (ASTNode*) chain);
////            continue;
////        }
//        // linking chain till chain_size - 1, last function call is not included
//        // last function call is not linked because it may not be valid and calling struct member
//        if(!chain->link(linker, nullptr, nullptr, 1, false, false)) {
//            continue;
//        }
//        auto call_parent = call->parent_val; // second last value
//        auto linked = call_parent->linked_node();
//        if(!linked) {
//            linker.error("unknown initializer call", (ASTNode*) chain);
//            continue;
//        }
//        auto linked_kind = linked->kind();
//        if(linked_kind == ASTNodeKind::StructMember || linked_kind == ASTNodeKind::UnnamedUnion || linked_kind == ASTNodeKind::UnnamedStruct) {
//            if(call->values.size() != 1) {
//                linker.error("expected a single value to initialize a struct member", (ASTNode*) chain);
//                continue;
//            }
//            auto base_def = linked->as_base_def_member_unsafe();
//            auto& value = call->values.front();
//            value->link(linker, value); // TODO send expected type by getting from base_def
//            initializers[base_def->name] = { false, value };
//            continue;
//        } else if(linked_kind == ASTNodeKind::FunctionDecl) {
//            // linking the last function call, since function call is valid
//            // call_ptr being sent as Value*&, if replaced other than ChainValue, it maybe invalid inside access chain
//            if(!call_ptr->link(linker, (Value*&) call_ptr, nullptr)) {
//                continue;
//            }
//            auto linked_func = linked->as_function();
//            auto func_parent = linked_func->parent();
//            auto called_struc = func_parent ? func_parent->as_struct_def() : nullptr;
//            if(!called_struc) {
//                linker.error("couldn't get struct of constructor in init block", (ASTNode*) chain);
//                continue;
//            }
//            bool found = false;
//            for(auto& inherit : mems_container->inherited) {
//                auto struc = inherit.type->get_direct_linked_struct();
//                if(struc && called_struc == struc) {
//                    found = true;
//                }
//            }
//            if(!found) {
//                linker.error((ASTNode*) chain) << "current struct doesn't inherit struct with name '" << called_struc->name_view() << "'";
//                continue;
//            }
//            initializers[called_struc->name_view()] = { true, chain };
//            continue;
//        } else {
//            linker.error("call to unknown node in init block", (ASTNode*) chain);
//        }
//    }
    diagnose_missing_members_for_init(linker);
}

void TryCatch::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    tryCall->link(linker, (Value*&) tryCall);
    if(catchScope.has_value()) {
        catchScope->link_sequentially(linker);
    }
}

void UnionDef::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    MembersContainer::declare_and_link(linker, node_ptr);
}

void UnsafeBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto prev = linker.safe_context;
    linker.safe_context = false;
    scope.link_sequentially(linker);
    linker.safe_context = prev;
}

void VariantCaseVariable::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    const auto member = member_param->parent();
    auto node = member->values.find(name);
    if(node == member->values.end()) {
        linker.error(this) << "variant case member variable not found in switch statement, name '" << name << "' not found";
        return;
    }
    member_param = node->second;
    linker.declare(name, this);
}

void WhileLoop::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    condition->link(linker, condition);
    body.link_sequentially(linker);
    linker.scope_end();
}

void ValueNode::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    value->link(linker, value);
}

void MultiFunctionNode::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {

    // link all the functions
    for(auto& func : functions) {
        func->declare_and_link(linker, (ASTNode*&) func);
    }

}
