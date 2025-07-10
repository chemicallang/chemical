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
#include "ast/types/VoidType.h"
#include "ast/values/NullValue.h"
#include "ast/values/StringValue.h"
#include "compiler/symres/SymbolResolver.h"
#include "DeclareTopLevel.h"
#include "ast/utils/ASTUtils.h"
#include "SymResLinkBody.h"
#include "SymResLinkBodyAPI.h"
#include "LinkSignatureAPI.h"

void sym_res_link_body(SymbolResolver& resolver, Scope* scope) {
    SymResLinkBody linker(resolver);
    linker.VisitScope(scope);
}

void sym_res_link_node_deprecated(SymbolResolver& resolver, ASTNode* node) {
    SymResLinkBody linker(resolver);
    linker.visit(node);
}

void SymResLinkBody::LinkMembersContainerNoScope(MembersContainer* container) {
    for(auto& inherits : container->inherited) {
        const auto def = inherits.type->get_members_container();
        if(def) {
            def->declare_inherited_members(linker);
        }
    }
    // this will only declare aliases
    container->declare_parsed_nodes(linker);
    // declare all the variables manually
    for (const auto var : container->variables()) {
        if(var->name.empty()) {
#ifdef DEBUG
            switch(var->kind()) {
                case ASTNodeKind::UnnamedStruct:
                case ASTNodeKind::UnnamedUnion:
                    break;
                default:
                    throw std::runtime_error("why does this variable has empty name");
            }
#endif
            continue;
        } else {
            linker.declare(var->name, var);
        }
    }
    // declare all the functions
    TopLevelDeclSymDeclare declarer(linker);

    for(auto& func : container->functions()) {
        declarer.visit(func);
    }
    for (const auto func: container->functions()) {
        visit(func);
    }
}

//void SymResLinkBody::VisitAccessChain(AccessChain *chain) {
//    chain->link(linker, nullptr, nullptr, false, false);
//}

void SymResLinkBody::VisitAssignmentStmt(AssignStatement *assign) {
    auto& lhs = assign->lhs;
    auto& value = assign->value;
    if(lhs->link_assign(linker, lhs, nullptr)) {
        BaseType* lhsType = lhs->create_type(linker.allocator);
        if(value->link(linker, value, lhsType)) {
            switch(assign->assOp){
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

void SymResLinkBody::VisitUsingStmt(UsingStmt* node) {
    if(!node->is_failed_chain_link()) {
        // we need to declare symbols once again, because all files in a module link signature
        // and then declare_and_link of all files is called, so after link_signature of each
        // file, symbols are dropped
        node->declare_symbols(linker);
    }
}

void SymResLinkBody::VisitBreakStmt(BreakStatement* node) {
    if(node->value) {
        node->value->link(linker, node->value);
    }
}

void SymResLinkBody::VisitDeleteStmt(DestructStmt* node) {
    auto& array_value = node->array_value;
    auto& identifier = node->identifier;
    if(array_value) {
        array_value->link(linker, array_value);
    }
    if(!identifier->link(linker, identifier)) {
        return;
    }
    auto type = identifier->get_canonical_type(linker.allocator);
    if(!type->is_pointer()) {
        linker.error("destruct cannot be called on a value that isn't a pointer", node);
        return;
    }
    auto found = linker.find("free");
    if(!found || !found->as_function()) {
        linker.error("'free' function should be declared before using destruct so calls can be made to it", node);
        return;
    }
    node->free_func_linked = found->as_function();
}

void SymResLinkBody::VisitProvideStmt(ProvideStmt* node) {
    auto& value = node->value;
    if(value->link(linker, value, nullptr)) {
        node->put_in(linker.implicit_args, value, &linker, [](ProvideStmt* stmt, void* data) {
            stmt->body.link_sequentially((*(SymbolResolver*) data));
        });
    }
}

void SymResLinkBody::VisitReturnStmt(ReturnStatement* node) {
    auto& value = node->value;
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

VariantCase* create_variant_case(SymResLinkBody& linker, SwitchStatement* stmt, VariantDefinition* def, FunctionCall* call) {
    auto& resolver = linker.linker;
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
                        linker.visit(variable);

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

void SymResLinkBody::VisitSwitchStmt(SwitchStatement *stmt) {

    auto& expression = stmt->expression;
    auto& scopes = stmt->scopes;
    auto& cases = stmt->cases;

    VariantDefinition* variant_def = nullptr;
    bool result = true;
    if(expression->link(linker, expression)) {
        variant_def = stmt->getVarDefFromExpr();
        if (variant_def && (scopes.size() < variant_def->variables().size() && !stmt->has_default_case())) {
            linker.error("expected all cases of variant in switch statement when no default case is specified", (ASTNode*) stmt);
            return;
        }
    } else {
        result = false;
    }

    if(result && stmt->is_value) {
        auto val_node = stmt->get_value_node();
        if(!val_node) {
            linker.error("expected a single value node for the value", (ASTNode*) stmt);
            return;
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
                            const auto varCase = create_variant_case(linker, stmt, variant_def, switch_case.first->as_identifier_unsafe());
                            if (varCase) {
                                switch_case.first = varCase;
                            }
                            continue;
                        }
                        case ValueKind::FunctionCall: {
                            const auto varCase = create_variant_case(*this, stmt, variant_def, switch_case.first->as_func_call_unsafe());
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
                                    const auto varCase = create_variant_case(*this, stmt, variant_def, chain->values.back()->as_func_call_unsafe());
                                    if(varCase) {
                                        switch_case.first = varCase;
                                    }
                                } else if(kind == ValueKind::Identifier) {
                                    const auto varCase = create_variant_case(linker, stmt, variant_def, chain->values.back()->as_identifier_unsafe());
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

}

bool SwitchStatement::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    SymResLinkBody temp_linker(linker);
    temp_linker.visit(this);
    return true;
}

void SymResLinkBody::VisitTypealiasStmt(TypealiasStatement* node) {
    if(!node->is_top_level()) {
        linker.declare_node(node->name_view(), node, node->specifier(), false);
        node->actual_type.link(linker);
    }
}

void SymResLinkBody::VisitVarInitStmt(VarInitStatement* node) {
    auto& type = node->type;
    auto& value = node->value;
    auto& attrs = node->attrs;
    if(node->is_top_level()) {
        if(attrs.signature_resolved && !type && value) {
            type = {value->create_type(*linker.ast_allocator), type.getLocation()};
        }
    } else {
        const auto type_resolved = !type || type.link(linker);
        const auto value_resolved = !value || value->link(linker, value, node->type_ptr_fast());
        if (!type_resolved || !value_resolved) {
            attrs.signature_resolved = false;
        }
        linker.declare(node->id_view(), node);
        if (attrs.signature_resolved) {
            if(value) {
                linker.current_func_type->mark_moved_value(linker.allocator, value, node->known_type(), linker, type != nullptr);
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

void SymResLinkBody::VisitComptimeBlock(ComptimeBlock* node) {
    node->body.link_sequentially(linker);
}

void SymResLinkBody::VisitDoWhileLoopStmt(DoWhileLoop* node) {
    linker.scope_start();
    node->body.link_sequentially(linker);
    node->condition->link(linker, node->condition);
    linker.scope_end();
}

void SymResLinkBody::VisitEnumMember(EnumMember* node) {
    if(node->init_value) {
        node->init_value->link(linker, node->init_value, nullptr);
    }
    linker.declare(node->name, node);
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

void SymResLinkBody::VisitEnumDecl(EnumDeclaration* node) {
    auto& members = node->members;
    auto& underlying_type = node->underlying_type;
    auto& underlying_integer_type = node->underlying_integer_type;
    const auto pure_underlying = underlying_type->pure_type(*linker.ast_allocator);
    const auto k = pure_underlying->kind();
    if(k == BaseTypeKind::IntN) {
        underlying_integer_type = pure_underlying->as_intn_type_unsafe();
    } else {
        const auto linked = pure_underlying->get_direct_linked_node();
        if(linked->kind() == ASTNodeKind::EnumDecl) {
            const auto inherited = linked->as_enum_decl_unsafe();
            configure_members_by_inheritance(node, inherited->next_start);
            underlying_integer_type = inherited->underlying_integer_type;
        } else {
            linker.error("given type is not an enum or integer type", node->encoded_location());
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

void SymResLinkBody::VisitForLoopStmt(ForLoop* node) {
    linker.scope_start();
    visit(node->initializer);
    node->conditionExpr->link(linker, node->conditionExpr);
    visit(node->incrementerExpr);
    node->body.link_sequentially(linker);
    linker.scope_end();
}

void SymResLinkBody::VisitFunctionParam(FunctionParam* node) {
    linker.declare(node->name, node);
}

void SymResLinkBody::VisitGenericTypeParam(GenericTypeParameter* node) {
    if(node->at_least_type) {
        node->at_least_type.link(linker);
    }
    node->declare_only(linker);
    if(node->def_type) {
        node->def_type.link(linker);
    }
}

void SymResLinkBody::VisitFunctionDecl(FunctionDeclaration* node) {
    if(node->body.has_value()) {
        // if has body declare params
        linker.scope_start();
        auto prev_func_type = linker.current_func_type;
        linker.current_func_type = node;
        for (const auto param : node->params) {
            visit(param);
        }
        if(node->FunctionType::data.signature_resolved) {
            if(node->is_comptime()) {
                linker.comptime_context = true;
            }
            node->body->link_sequentially(linker);
            linker.comptime_context = false;
        }
        linker.scope_end();
        linker.current_func_type = prev_func_type;
    }

}

void SymResLinkBody::VisitInterfaceDecl(InterfaceDefinition* node) {
    LinkMembersContainer(node);
}

void SymResLinkBody::VisitStructDecl(StructDefinition* node) {
    LinkMembersContainer(node);
    node->register_use_to_inherited_interfaces(node);
}

void SymResLinkBody::VisitVariantDecl(VariantDefinition* node) {
    LinkMembersContainer(node);
}

void SymResLinkBody::VisitCapturedVariable(CapturedVariable* node) {
    const auto found = linker.find(node->name);
    if(found != nullptr) {
        node->linked = found;
    }
    linker.declare(node->name, node);
}

void SymResLinkBody::VisitGenericFuncDecl(GenericFuncDecl* node) {
    // symbol resolve the master declaration
    linker.scope_start();
    for(const auto param : node->generic_params) {
        visit(param);
    }
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    visit(node->master_impl);
    linker.generic_context = prev_gen_context;
    linker.scope_end();
    node->body_linked = true;
    // finalizing the body of every function that was instantiated before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(node, node->instantiations);
}

void SymResLinkBody::VisitGenericImplDecl(GenericImplDecl* node) {
    auto& generic_params = node->generic_params;
    linker.scope_start();
    for(const auto param : generic_params) {
        visit(param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    visit(node->master_impl);
    linker.generic_context = prev_gen_context;
    linker.scope_end();
    node->body_linked = true;
    // finalizing body of instantiations that occurred before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(node, node->instantiations);
}

void SymResLinkBody::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    linker.scope_start();
    for(const auto param : node->generic_params) {
        visit(param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    visit(node->master_impl);
    linker.generic_context = prev_gen_context;
    linker.scope_end();
    node->body_linked = true;
    // finalizing body of instantiations that occurred before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(node, node->instantiations);
}

void SymResLinkBody::VisitGenericStructDecl(GenericStructDecl* node) {
    linker.scope_start();
    for(const auto param : node->generic_params) {
        visit(param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    visit(node->master_impl);
    linker.generic_context = prev_gen_context;
    linker.scope_end();
    node->body_linked = true;
    // finalizing body of instantiations that occurred before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(node, node->instantiations);
}

void SymResLinkBody::VisitGenericUnionDecl(GenericUnionDecl* node) {
    linker.scope_start();
    for(const auto param : node->generic_params) {
        visit(param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    visit(node->master_impl);
    linker.generic_context = prev_gen_context;
    linker.scope_end();
    node->body_linked = true;
    // finalizing body of instantiations that occurred before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(node, node->instantiations);
}

void SymResLinkBody::VisitGenericVariantDecl(GenericVariantDecl* node) {
    linker.scope_start();
    for(const auto param : node->generic_params) {
        visit(param);
    }
    // declare and link, but don't generate any default constructors / destructors / such things
    const auto prev_gen_context = linker.generic_context;
    linker.generic_context = true;
    visit(node->master_impl);
    linker.generic_context = prev_gen_context;
    linker.scope_end();
    node->body_linked = true;
    // finalizing body of instantiations that occurred before declare_and_link
    auto& allocator = *linker.ast_allocator;
    for(const auto inst : node->instantiations) {
        node->finalize_body(allocator, inst);
    }
    // finalize the body of all instantiations
    // this basically visits the instantiations body and makes the types concrete
    linker.genericInstantiator.FinalizeBody(node, node->instantiations);
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

void SymResLinkBody::VisitIfStmt(IfStatement* node) {

    if(node->is_top_level()) {
        auto scope = node->get_evaluated_scope_by_linking(linker);
        if(scope) {
            visit(scope);
        }
        return;
    }

    node->link_conditions_no_patt_match_expr(linker);

    if(node->is_computable || node->compile_time_computable()) {
        node->is_computable = true;
        if(node->computed_scope.has_value()) {
            auto scope = node->computed_scope.value();
            if(scope) {
                visit(scope);
            }
            return;
        }
        if(!linker.comptime_context) {
            auto condition_val = node->resolved_condition ? node->get_condition_const((InterpretScope&) linker.comptime_scope) : std::optional(false);
            if (condition_val.has_value()) {
                auto eval = node->get_evaluated_scope((InterpretScope&) linker.comptime_scope, &linker, condition_val.value());
                node->computed_scope = eval;
                if (eval) {
                    visit(eval);
                }
                return;
            } else {
                node->is_computable = false;
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
    link_body(node->ifBody, node->condition, linker, moved_ids, moved_chains);
    // link the else ifs
    for(auto& elseIf : node->elseIfs) {
        link_body(elseIf.second, elseIf.first, linker, moved_ids, moved_chains);
    }
    // link the else body
    if(node->elseBody.has_value()) {
        linker.scope_start();
        linker.link_body_seq_backing_moves(node->elseBody.value(), moved_ids, moved_chains);
        linker.scope_end();
    }

    curr_func->restore_moved_ids(moved_ids);
    curr_func->restore_moved_chains(moved_chains);
}

bool IfStatement::link(SymbolResolver &linker, Value* &value_ptr, BaseType *expected_type) {
    // TODO: remove this instance
    SymResLinkBody temp_linker(linker);
    temp_linker.visit(this);
    return true;
}

void SymResLinkBody::VisitImplDecl(ImplDefinition* node) {
    const auto linked_node = node->interface_type->linked_node();
    if(!linked_node) {
        return;
    }
    const auto linked = linked_node->as_interface_def();
    if(!linked) {
        return;
    }
    linker.scope_start();
    const auto struct_linked = node->struct_type ? node->struct_type->linked_struct_def() : nullptr;
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
    LinkMembersContainerNoScope(node);
    linker.scope_end();
}

void SymResLinkBody::VisitNamespaceDecl(Namespace* node) {
    linker.scope_start();
    if(node->root) {
        node->root->declare_extended_in_linker(linker);
    } else {
        TopLevelDeclSymDeclare declarer(linker);
        for(auto& child : node->nodes) {
            declarer.visit(child);
        }
    }
    for(const auto child : node->nodes) {
        visit(child);
    }
    linker.scope_end();
}

void SymResLinkBody::VisitScope(Scope* node) {
    for (const auto child: node->nodes) {
        visit(child);
    }
}

void SymResLinkBody::VisitLoopBlock(LoopBlock* node) {
    node->body.link_sequentially(linker);
}

void SymResLinkBody::VisitInitBlock(InitBlock* node) {
    auto mems_container = node->getContainer();
    if(!mems_container) {
        linker.error("unexpected init block", node);
        return;
    }
    for(auto& in : node->initializers) {
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
    node->diagnose_missing_members_for_init(linker);
}

// void TryCatch::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
//    // Try catch not supported !
//    tryCall->link(linker, (Value*&) tryCall);
//    if(catchScope.has_value()) {
//        catchScope->link_sequentially(linker);
//    }
// }

void SymResLinkBody::VisitUnionDecl(UnionDef* node) {
    LinkMembersContainer(node);
}

void SymResLinkBody::VisitUnsafeBlock(UnsafeBlock* node) {
    auto prev = linker.safe_context;
    linker.safe_context = false;
    node->scope.link_sequentially(linker);
    linker.safe_context = prev;
}

void SymResLinkBody::VisitVariantCaseVariable(VariantCaseVariable* node) {
    const auto member = node->member_param->parent();
    auto child = member->values.find(node->name);
    if(child == member->values.end()) {
        linker.error(node) << "variant case member variable not found in switch statement, name '" << node->name << "' not found";
        return;
    }
    node->member_param = child->second;
    linker.declare(node->name, node);
}

void SymResLinkBody::VisitWhileLoopStmt(WhileLoop* node) {
    linker.scope_start();
    node->condition->link(linker, node->condition);
    node->body.link_sequentially(linker);
    linker.scope_end();
}

void SymResLinkBody::VisitValueNode(ValueNode* node) {
    node->value->link(linker, node->value);
}

void SymResLinkBody::VisitMultiFunctionNode(MultiFunctionNode* node) {

    // link all the functions
    for(const auto func : node->functions) {
        visit(func);
    }

}

void SymResLinkBody::VisitValueWrapper(ValueWrapperNode* node) {
    node->value->link(linker, node->value);
}

void SymResLinkBody::VisitEmbeddedNode(EmbeddedNode* node) {
    node->sym_res_fn(&linker, node);
}

// --------------- Values and Types BEGIN HERE -----------------
// -------------------------------------------------------------
// -------------------------------------------------------------


inline bool link_val(SymbolResolver &linker, Value* value, Value** value_ptr, BaseType* expected_type, bool assign) {
    if(assign && value->kind() == ValueKind::Identifier) {
        return value->as_identifier_unsafe()->link_assign(linker, *value_ptr, expected_type);
    } else {
        return value->link(linker, *value_ptr, expected_type);
    }
}

bool AccessChain::link(SymbolResolver &linker, BaseType *expected_type, Value** value_ptr, bool check_validity, bool assign) {

    if(!link_val(linker, values[0], value_ptr, values.size() == 1 ? expected_type : nullptr, assign)) {
        return false;
    }

    // auto prepend self identifier, if not present and linked with struct member, anon union or anon struct
    auto linked = values[0]->linked_node();
    if(linked) {
        const auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::StructMember || linked_kind == ASTNodeKind::UnnamedUnion || linked_kind == ASTNodeKind::UnnamedStruct) {
            if (!linker.current_func_type) {
                linker.error(values[0]) << "unresolved identifier with struct member / function, with name '" << values[0]->representation() << '\'';
                return false;
            }
            auto self_param = linker.current_func_type->get_self_param();
            if (!self_param) {
                auto decl = linker.current_func_type->as_function();
                if (!decl || !decl->is_constructor_fn() && !decl->is_comptime()) {
                    linker.error(values[0]) << "unresolved identifier '" << values[0]->representation() << "', because function doesn't take a self argument";
                    return false;
                }
            }
        }
    }

    if(values.size() == 1) {
        return true;
    }

    const auto values_size = values.size();
    if (values_size > 1) {
        const auto last = values_size - 1;
        unsigned i = 1;
        while (i < values_size) {
            if(!values[i]->as_identifier_unsafe()->find_link_in_parent(values[i - 1], linker, i == last ? expected_type : nullptr)) {
                return false;
            }
            i++;
        }
    }

    if(check_validity && linker.current_func_type) {
        // check chain for validity, if it's moved or members have been moved
        linker.current_func_type->check_chain(this, assign, linker);
    }

    return true;
}

bool LoopBlock::link(SymbolResolver &linker, Value* &value_ptr, BaseType *expected_type) {
    body.link_sequentially(linker);
    return true;
}

bool VariantCase::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    return true;
}

bool ArrayType::link(SymbolResolver &linker, SourceLocation loc) {
    const auto type_linked = elem_type.link(linker);
    if(!type_linked) return false;
    if(array_size_value) {
        if(array_size_value->link(linker, array_size_value)) {
            const auto evaluated = array_size_value->evaluated_value(linker.comptime_scope);
            const auto number = evaluated->get_the_number();
            if(number.has_value()) {
                array_size = number.value();
                return true;
            }
        }
        return false;
    }
    return true;
}

bool DynamicType::link(SymbolResolver &linker, SourceLocation loc) {
    return referenced->link(linker, loc);
}

bool FunctionType::link(SymbolResolver &linker, SourceLocation loc) {
    bool resolved = true;
    for (auto &param: params) {
        if(!param->link_param_type(linker)) {
            resolved = false;
        }
    }
    if(!returnType.link(linker)) {
        resolved = false;
    }
    if(resolved) {
        data.signature_resolved = true;
    }
    return resolved;
}

bool GenericType::link(SymbolResolver &linker, SourceLocation loc) {
    const auto res = referenced->link(linker, loc);
    if(!res) return false;
    for(auto& type : types) {
        if(!type.link(linker)) {
            return false;
        }
    }
    return instantiate(linker.genericInstantiator, loc);
}

bool NamedLinkedType::link(SymbolResolver &linker, SourceLocation loc) {
    const auto found = linker.find(link_name);
    if(found) {
        linked = found;
    } else if(linked == nullptr) {
        linker.error(loc) << "unresolved symbol, couldn't find referenced type '" << link_name << '\'';
        return false;
    }
    return true;
}

bool PointerType::link(SymbolResolver &linker, SourceLocation loc) {
    return type->link(linker, loc);
}

bool StructType::link(SymbolResolver &linker, SourceLocation loc) {
    take_variables_from_parsed_nodes(linker);
    sym_res_vars_signature(linker, this);
    if(!name.empty()) {
        linker.declare(name, this);
    }
    return true;
}

bool UnionType::link(SymbolResolver &linker, SourceLocation loc) {
    take_variables_from_parsed_nodes(linker);
    sym_res_vars_signature(linker, this);
    if(!name.empty()) {
        linker.declare(name, this);
    }
    return true;
}

bool AddrOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    auto res = value->link(linker, value);
    if(res) {

        // reporting parameters that their address has been taken
        // which allows them to generate a variable and store themselves onto it
        // if they are of integer types
        // before the taking of address, the variables act immutable
        const auto linked = value->linked_node();
        if(linked) {
            switch (linked->kind()) {
                case ASTNodeKind::FunctionParam:
                    linked->as_func_param_unsafe()->set_has_address_taken(true);
                    break;
                default:
                    break;
            }
        }
        if(!linker.linking_signature) {
            // TODO we must perform mutability checks during link signature
            is_value_mutable = value->check_is_mutable(linker.allocator, true);
        }
    }
    return res;
}

bool ArrayValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    auto& elemType = known_elem_type();
    if(elemType) {
        elemType.link(linker);
    }
    if(expected_type && expected_type->kind() == BaseTypeKind::Array) {
        const auto arr_type = (ArrayType*) expected_type;
        elemType = arr_type->elem_type.copy(*linker.ast_allocator);
    }
    if(elemType) {
        const auto def = elemType->linked_struct_def();
        if(def) {
            unsigned i = 0;
            while (i < values.size()) {
                auto& val_ptr = values[i];
                const auto value = val_ptr;
                value->link(linker, val_ptr, elemType);
                const auto implicit = def->implicit_constructor_func(linker.allocator, value);
                if(implicit) {
                    link_with_implicit_constructor(implicit, linker, value);
                } else if(!linker.linking_signature && !elemType->satisfies(linker.allocator, value, false)) {
                    linker.unsatisfied_type_err(value, elemType);
                }
                i++;
            }
            return true;
        }
    }
    auto& current_func_type = *linker.current_func_type;
    auto& known_elem_type = elemType;
    unsigned i = 0;
    for(auto& value : values) {
        const auto link_res = value->link(linker, value, nullptr);
        if(link_res && i == 0 && !known_elem_type) {
            known_elem_type = TypeLoc(value->known_type(), known_elem_type.getLocation());
        }
        if(known_elem_type) {
            if(!linker.linking_signature) {
                current_func_type.mark_moved_value(linker.allocator, value, known_elem_type, linker, elemType != nullptr);
                if(!known_elem_type->satisfies(linker.allocator, value, false)) {
                    linker.unsatisfied_type_err(value, known_elem_type);
                }
            }
        }
        i++;
    }
    return true;
}

bool BlockValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    if(scope.nodes.empty()) {
        linker.error("empty block value not allowed", this);
        return false;
    } else {
        scope.link_sequentially(linker);
    }
    const auto lastNode = scope.nodes.back();
    const auto lastKind = lastNode->kind();
    if(lastKind != ASTNodeKind::ValueWrapper) {
        linker.error("block doesn't contain a value wrapper as last node", this);
        return false;
    }
    const auto lastValNode = lastNode->as_value_wrapper_unsafe();
    calculated_value = lastValNode->value;
    // TODO return appropriate result by getting it from scope
    return true;
}

bool CastedValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType* expected_type) {
    if(type.link(linker)) {
        if(value->link(linker, value, type)) {
            return true;
        }
    }
    return false;
}

bool DereferenceValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    if(linker.safe_context) {
        linker.warn("dereferencing a pointer in safe context is prohibited", this);
    }
    return value->link(linker, value);
}

bool Expression::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    auto f = firstValue->link(linker, firstValue);
    auto s = secondValue->link(linker, secondValue);
    auto result = f && s;
    // ast allocator is being used
    // it's unknown when this expression should be disposed
    // file level / module level allocator should be used, when this expression belongs to a function
    // or decl that is private or internal, however that is hard to determine
    if(!linker.linking_signature) {
        // TODO this created_type should always be created, however this creates an error
        created_type = create_type(*linker.ast_allocator);
    }
    return result;
}

bool IndexOperator::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    const auto linked = parent_val->link(linker, (Value*&) parent_val, nullptr);
    for(auto& value : values) {
        if(!value->link(linker, value)) {
            return false;
        }
    }
    return linked;
}


bool IsValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    const auto a = value->link(linker, value);
    const auto b = type.link(linker);
    return a && b;
}

BaseType* find_return_type(ASTAllocator& allocator, std::vector<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        if(node->as_return() != nullptr) {
            auto returnStmt = node->as_return();
            if(returnStmt->value) {
                const auto created = returnStmt->value->create_type(allocator);
                if(created) {
                    return created;
                }
            } else {
                return new (allocator.allocate<VoidType>()) VoidType();
            }
        } else {
            const auto loop_ast = node->as_loop_ast();
            if(loop_ast) {
                auto found = find_return_type(allocator, node->as_loop_ast()->body.nodes);
                if (found != nullptr) {
                    return found;
                }
            }
        }
    }
    return new (allocator.allocate<VoidType>()) VoidType();
}

bool link_params_and_caps(LambdaFunction* fn, SymbolResolver &linker, bool link_param_types) {
    // TODO rely on visiting the variables, and remove this
    SymResLinkBody temp_linker(linker);
    for(const auto cap : fn->captureList) {
        temp_linker.visit(cap);
    }
    bool result = true;
    for (auto& param : fn->params) {
        if(link_param_types) {
            if(!param->link_param_type(linker)) {
                result = false;
            }
        }
        temp_linker.visit(param);
    }
    return result;
}

bool link_full(LambdaFunction* fn, SymbolResolver &linker, bool link_param_types) {
    linker.scope_start();
    const auto result = link_params_and_caps(fn, linker, link_param_types);
    fn->scope.link_sequentially(linker);
    linker.scope_end();
    return result;
}

bool LambdaFunction::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {

    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = this;

    auto func_type = expected_type ? expected_type->canonical()->get_function_type() : nullptr;

    if(!func_type) {

#ifdef DEBUG
        linker.info("deducing lambda function type by visiting body", (Value*) this);
#endif

        // linking params and their types
        auto result = link_full(this, linker, true);

        // finding return type
        auto found_return_type = find_return_type(*linker.ast_allocator, scope.nodes);

        returnType = {found_return_type, get_location()};

        if(result) {
            data.signature_resolved = true;
        }

    } else {

        if(!params.empty()) {
            auto& param = params[0];
            if((param->name == "self" || param->name == "this") && (!param->type || param->type->kind() == BaseTypeKind::Void)) {
                param->type = func_type->params[0]->type;
            }
        }

        link(linker, func_type);
        if(link_full(this, linker, false)) {
            data.signature_resolved = true;
        }

    }

    if(!captureList.empty()) {

        if(prev_func_type) {
            for (const auto captured: captureList) {
                if (captured->capture_by_ref) {
                    continue;
                }
                // we have to allocate an identifier to mark it moved
                // maybe design for this should change a little
                // TODO: this identifier doesn't allow us to check if value has been moved prior
                // because is_moved is used to check
                const auto identifier = new(linker.ast_allocator->allocate<VariableIdentifier>()) VariableIdentifier(
                        captured->name, captured->encoded_location(), false
                );
                identifier->linked = captured->linked;
                // we must move the identifiers in capture list
                prev_func_type->mark_moved_value(linker.allocator, identifier, captured->linked->known_type(), linker, false);
            }
        }

        setIsCapturing(true);

    }

    linker.current_func_type = prev_func_type;

    return true;

}

void copy_func_params_types(const std::vector<FunctionParam*>& from_params, std::vector<FunctionParam*>& to_params, SymbolResolver& resolver, Value* debug_value) {
    if(to_params.size() > from_params.size()) {
        resolver.error(debug_value) << "Lambda function type expects " << std::to_string(from_params.size()) << " however given " << std::to_string(to_params.size());
        return;
    }
    auto total = from_params.size();
    auto start = 0;
    while(start < total) {
        const auto from_param = from_params[start];
        if(start >= to_params.size()) {
            to_params.emplace_back(nullptr);
        }
        const auto to_param = to_params[start];
        if(!to_param || !to_param->type || to_param->is_implicit() || from_param->is_implicit()) {
            const auto copied = from_param->copy(*resolver.ast_allocator);
            // change the name to what user wants
            if(to_param) {
                copied->name = to_param->name;
            }
            to_params[start] = copied;
        } else {
            // link the given parameter
            to_param->link_param_type(resolver);
            // check it's type is same as the from parameter
            if(!to_param->type->is_same(from_param->type)) {
                resolver.error(debug_value) << "Lambda function param at index " << std::to_string(start) << " with type " << from_param->type->representation() << ", redeclared with type " << to_param->type->representation();
            }
        }
        start++;
    }
}

bool LambdaFunction::link(SymbolResolver &linker, FunctionType* func_type) {
    copy_func_params_types(func_type->params, params, linker, this);
    if(!returnType) {
        returnType = func_type->returnType.copy(*linker.ast_allocator);
    } else if(!returnType->is_same(func_type->returnType)) {
        linker.error((Value*) this) << "Lambda function type expected return type to be " << func_type->returnType->representation() << " but got lambda with return type " << returnType->representation();
    }
    setIsCapturing(func_type->isCapturing());
    return true;
}

bool NegativeValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    return value->link(linker, value);
}

bool NewTypedValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    return type.link(linker);
}

bool NewValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    return value->link(linker, value, nullptr);
}

bool PlacementNewValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    const auto a = pointer->link(linker, pointer, nullptr);
    const auto b = value->link(linker, value, nullptr);
    return a && b;
}

bool NotValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    return value->link(linker, value);
}

bool NullValue::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    if(expected_type) {
        const auto kind = expected_type->kind();
        if(kind == BaseTypeKind::Function || kind == BaseTypeKind::Pointer) {
            expected = expected_type->copy(*linker.ast_allocator);
        }
    }
    return true;
}

bool PatternMatchExpr::link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type) {
    if(!expression->link(linker, expression, nullptr)) {
        return false;
    }
    const auto child_member = find_member_from_expr(linker.allocator, linker);
    if(!child_member) {
        return false;
    }
    // set the member, so we don't need to resolve it again
    member = child_member;
    auto& params = child_member->values;
    if(elseExpression.kind == PatternElseExprKind::DefValue && param_names.size() != 1) {
        linker.error("must destructure one member for default value to work", this);
        return false;
    }
    for(const auto nameId : param_names) {
        auto found = params.find(nameId->identifier);
        if(found == params.end()) {
            linker.error("couldn't find parameter in variant member", nameId);
        } else {
            nameId->member_param = found->second;
            // we declare this id, so anyone can link with it
            linker.declare(nameId->identifier, nameId);
        }
    }
    auto& elseVal = elseExpression.value;
    if (elseVal && !elseVal->link(linker, elseVal, nullptr)) {
        return false;
    }
}

bool SizeOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    for_type.link(linker);
    return true;
}

bool AlignOfValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type) {
    for_type.link(linker);
    return true;
}


bool StringValue::link(SymbolResolver &linker, Value*& value_ptr, BaseType *type) {
    if(type && type->kind() == BaseTypeKind::Array) {
        is_array = true;
        auto arrayType = (ArrayType*) (type);
        if(arrayType->get_array_size() > value.size()) {
            length = (unsigned int) arrayType->get_array_size();
        } else if(arrayType->has_no_array_size()) {
            length = value.size() + 1; // adding 1 for the last /0
        } else {
#ifdef DEBUG
            throw std::runtime_error("unknown");
#endif
        }
    }
    return true;
}

bool StructValue::link(SymbolResolver& linker, Value*& value_ptr, BaseType* expected_type) {
    if(refType) {
        if(!refType.link(linker)) {
            return false;
        }
    } else {
        if(!expected_type) {
            linker.error("unnamed struct value cannot link without a type", this);
            refType = { new (linker.ast_allocator->allocate<StructType>()) StructType("", nullptr, encoded_location()), encoded_location()};
            return false;
        }
        refType = {expected_type, refType.getLocation()};
    }
    if(!resolve_container(linker.genericInstantiator)) {
        return false;
    }
    diagnose_missing_members_for_init(linker);
    if(!allows_direct_init()) {
        linker.error(this) << "struct value with a constructor cannot be initialized, name '" << definition->name_view() << "' has a constructor";
    }
    auto refTypeKind = refType->kind();
    if(refTypeKind == BaseTypeKind::Generic) {
        for (auto& arg: generic_list()) {
            arg.link(linker);
        }
    }
    auto& current_func_type = *linker.current_func_type;
    // linking values
    for (auto &val: values) {
        auto& val_ptr = val.second.value;
        const auto value = val_ptr;
        auto child_node = container->child_member_or_inherited_struct(val.first);
        if(!child_node) {
            linker.error(this) << "unresolved child '" << val.first << "' in struct declaration";
            continue;
        }
        auto child_type = child_node->known_type();
        const auto val_linked = val_ptr->link(linker, val_ptr, child_type);
        const auto member = container->direct_variable(val.first);
        if(val_linked && member) {
            const auto mem_type = member->known_type();
            if(!linker.linking_signature) {
                current_func_type.mark_moved_value(linker.allocator, val.second.value, mem_type, linker);
            }
            auto implicit = mem_type->implicit_constructor_for(linker.allocator, val_ptr);
            if(implicit) {
                link_with_implicit_constructor(implicit, linker, val_ptr);
            } else if(!linker.linking_signature && !mem_type->satisfies(linker.allocator, value, false)) {
                linker.unsatisfied_type_err(value, mem_type);
            }
        }
    }
    return true;
}

bool VariableIdentifier::link(SymbolResolver &linker, bool check_access, Value** ptr_ref) {
    linked = linker.find(value);
    if(linked) {
        if(linked->kind() == ASTNodeKind::GenericTypeParam) {
            if(ptr_ref) {
                auto& allocator = *linker.ast_allocator;
                const auto linked_type = new (allocator.allocate<LinkedType>()) LinkedType(linked);
                *ptr_ref = new (allocator.allocate<TypeInsideValue>()) TypeInsideValue(linked_type, encoded_location());
                return true;
            } else {
                linker.error(this) << "cannot replace identifier '" << value << "' that references a generic type parameter";
            }
        } else {
            if (check_access && linker.current_func_type) {
                // check for validity if accessible or assignable (because moved)
                linker.current_func_type->check_id(this, linker);
            }
            process_linked(&linker);
        }
        return true;
    } else {
        linker.error(this) << "unresolved variable identifier '" << value << "' not found";
    }
    return false;
}
