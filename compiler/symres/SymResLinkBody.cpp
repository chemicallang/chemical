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
#include "ast/values/NumberValue.h"
#include "ast/types/LinkedValueType.h"
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

void sym_res_link_value_deprecated(SymbolResolver& resolver, Value* value, BaseType* expected_type) {
    SymResLinkBody linker(resolver);
    linker.visit(value, expected_type);
}

void sym_res_link_type_deprecated(SymbolResolver& resolver, BaseType* type, SourceLocation loc) {
    SymResLinkBody linker(resolver);
    linker.visit(type, loc);
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

void link_assignable(SymResLinkBody& symRes, Value* lhs, BaseType* expected_type) {
    switch(lhs->kind()) {
        case ValueKind::Identifier:
            symRes.expected_type = expected_type;
            symRes.VisitVariableIdentifier(lhs->as_identifier_unsafe(), false);
            break;
        case ValueKind::AccessChain:
            symRes.expected_type = expected_type;
            symRes.VisitAccessChain(lhs->as_access_chain_unsafe(), true, true);
            break;
        default:
            symRes.visit(lhs, expected_type);
    }
}

inline void link_val(SymResLinkBody &symRes, Value* value, BaseType* expected_type, bool assign) {
    if(assign && value->kind() == ValueKind::Identifier) {
        symRes.expected_type = expected_type;
        symRes.VisitVariableIdentifier(value->as_identifier_unsafe(), false);
    } else {
        symRes.visit(value, expected_type);
    }
}

void SymResLinkBody::VisitAccessChain(AccessChain* chain, bool check_validity, bool assignment) {
    // load the expected type beforehand
    const auto exp_type = expected_type;
    auto& values = chain->values;

    link_val(*this, values[0], values.size() == 1 ? exp_type : nullptr, assignment);

    // auto prepend self identifier, if not present and linked with struct member, anon union or anon struct
    auto linked = values[0]->linked_node();
    if(linked) {
        const auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::StructMember || linked_kind == ASTNodeKind::UnnamedUnion || linked_kind == ASTNodeKind::UnnamedStruct) {
            if (!linker.current_func_type) {
                linker.error(values[0]) << "unresolved identifier with struct member / function, with name '" << values[0]->representation() << '\'';
                return;
            }
            auto self_param = linker.current_func_type->get_self_param();
            if (!self_param) {
                auto decl = linker.current_func_type->as_function();
                if (!decl || !decl->is_constructor_fn() && !decl->is_comptime()) {
                    linker.error(values[0]) << "unresolved identifier '" << values[0]->representation() << "', because function doesn't take a self argument";
                    return;
                }
            }
        }
    }

    if(values.size() == 1) {
        return;
    }

    const auto values_size = values.size();
    if (values_size > 1) {
        const auto last = values_size - 1;
        unsigned i = 1;
        while (i < values_size) {
            if(!values[i]->as_identifier_unsafe()->find_link_in_parent(values[i - 1], linker, i == last ? exp_type : nullptr)) {
                return;
            }
            i++;
        }
    }

    if(check_validity && linker.current_func_type) {
        // check chain for validity, if it's moved or members have been moved
        linker.current_func_type->check_chain(chain, assignment, linker);
    }
}

void SymResLinkBody::VisitVariableIdentifier(VariableIdentifier* identifier, bool check_access) {
    auto& value = identifier->value;
    identifier->linked = linker.find(value);
    if(identifier->linked) {
//        if(linked->kind() == ASTNodeKind::GenericTypeParam) {
//            if(ptr_ref) {
//                auto& allocator = *linker.ast_allocator;
//                const auto linked_type = new (allocator.allocate<LinkedType>()) LinkedType(linked);
//                *ptr_ref = new (allocator.allocate<TypeInsideValue>()) TypeInsideValue(linked_type, encoded_location());
//                return true;
//            } else {
//                linker.error(this) << "cannot replace identifier '" << value << "' that references a generic type parameter";
//            }
//        } else {
        if (check_access && linker.current_func_type) {
            // check for validity if accessible or assignable (because moved)
            linker.current_func_type->check_id(identifier, linker);
        }
        identifier->process_linked(&linker);
//        }
        return;
    } else {
        linker.error(identifier) << "unresolved variable identifier '" << value << "' not found";
    }
}

void SymResLinkBody::VisitAssignmentStmt(AssignStatement *assign) {
    auto& lhs = assign->lhs;
    auto& value = assign->value;

    link_assignable(*this, lhs, nullptr);

    BaseType* lhsType = lhs->create_type(linker.allocator);

    visit(value, lhsType);

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
        // TODO: we may need to pass expected type from
        //    where it is being assigned
        visit(node->value);
    }
}

void SymResLinkBody::VisitDeleteStmt(DestructStmt* node) {
    auto& array_value = node->array_value;
    auto& identifier = node->identifier;
    if(array_value) {
        visit(array_value);
    }
    visit(identifier);
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
    visit(value);
    node->put_in(linker.implicit_args, value, &linker, [](ProvideStmt* stmt, void* data) {
        stmt->body.link_sequentially((*(SymbolResolver*) data));
    });
}

void SymResLinkBody::VisitReturnStmt(ReturnStatement* node) {
    auto& value = node->value;
    if (value) {

        const auto func_type = linker.current_func_type;
        visit(value, func_type->returnType ? func_type->returnType : nullptr);

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

    visit(expression);

    variant_def = stmt->getVarDefFromExpr();
    if (variant_def && (scopes.size() < variant_def->variables().size() && !stmt->has_default_case())) {
        linker.error("expected all cases of variant in switch statement when no default case is specified", (ASTNode*) stmt);
        return;
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
                    visit(switch_case.first);
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

void SymResLinkBody::VisitTypealiasStmt(TypealiasStatement* node) {
    if(!node->is_top_level()) {
        linker.declare_node(node->name_view(), node, node->specifier(), false);
        visit(node->actual_type);
    }
}

void SymResLinkBody::VisitVarInitStmt(VarInitStatement* node) {
    if(node->type) {
        visit(node->type);
    }
    if(node->value) {
        // TODO: if type didn't link successfully we need to NOT use it
        visit(node->value, node->type_ptr_fast());
    }
    auto& type = node->type;
    auto& value = node->value;
    auto& attrs = node->attrs;
    if(node->is_top_level()) {
        if(attrs.signature_resolved && !type && value) {
            type = {value->create_type(*linker.ast_allocator), type.getLocation()};
        }
    } else {
        if(type) {
            visit(type);
        }
        if(value) {
            visit(value, node->type_ptr_fast());
        }
        const auto type_resolved = true;
        const auto value_resolved = true;
//        if (!type_resolved || !value_resolved) {
//            attrs.signature_resolved = false;
//        }
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
    visit(node->condition);
    linker.scope_end();
}

void SymResLinkBody::VisitEnumMember(EnumMember* node) {
    if(node->init_value) {
        visit(node->init_value);
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
            visit(value);
        }
    }
    linker.scope_end();
}

void SymResLinkBody::VisitForLoopStmt(ForLoop* node) {
    linker.scope_start();
    visit(node->initializer);
    visit(node->conditionExpr);
    visit(node->incrementerExpr);
    node->body.link_sequentially(linker);
    linker.scope_end();
}

void SymResLinkBody::VisitFunctionParam(FunctionParam* node) {
    linker.declare(node->name, node);
}

void SymResLinkBody::VisitGenericTypeParam(GenericTypeParameter* node) {
    if(node->at_least_type) {
        visit(node->at_least_type);
    }
    node->declare_only(linker);
    if(node->def_type) {
        visit(node->def_type);
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
        SymResLinkBody& symRes,
        std::vector<VariableIdentifier*>& moved_ids,
        std::vector<AccessChain*>& moved_chains
) {
    auto& linker = symRes.linker;
    linker.scope_start();
    if(conditionExpr->kind() == ValueKind::PatternMatchExpr) {
        // we must link it here
        // since pattern matching introduces symbols to link against
        symRes.visit(conditionExpr);
    }
    linker.link_body_seq_backing_moves(body, moved_ids, moved_chains);
    linker.scope_end();
}

void link_conditions(IfStatement* stmt, SymResLinkBody& symRes) {
    symRes.visit(stmt->condition);
    for (auto& cond: stmt->elseIfs) {
        symRes.visit(cond.first);
    }
}

bool IfStatement::link_conditions(SymbolResolver &linker) {
    SymResLinkBody temp(linker);
    ::link_conditions(this, temp);
    return true;
}

void link_conditions_no_patt_match_expr(IfStatement* stmt, SymResLinkBody &symRes) {
    if(stmt->condition->kind() != ValueKind::PatternMatchExpr) {
        symRes.visit(stmt->condition);
    }
    for (auto& cond: stmt->elseIfs) {
        if(cond.first->kind() != ValueKind::PatternMatchExpr) {
            symRes.visit(cond.first);
        }
    }
}

void SymResLinkBody::VisitIfStmt(IfStatement* node) {

    if(node->is_top_level()) {
        auto scope = node->get_evaluated_scope_by_linking(linker);
        if(scope) {
            visit(scope);
        }
        return;
    }

    link_conditions_no_patt_match_expr(node, *this);

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
    link_body(node->ifBody, node->condition, *this, moved_ids, moved_chains);
    // link the else ifs
    for(auto& elseIf : node->elseIfs) {
        link_body(elseIf.second, elseIf.first, *this, moved_ids, moved_chains);
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
        visit(in.second.value);
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
    visit(node->condition);
    node->body.link_sequentially(linker);
    linker.scope_end();
}

void SymResLinkBody::VisitValueNode(ValueNode* node) {
    visit(node->value);
}

void SymResLinkBody::VisitMultiFunctionNode(MultiFunctionNode* node) {

    // link all the functions
    for(const auto func : node->functions) {
        visit(func);
    }

}

void SymResLinkBody::VisitValueWrapper(ValueWrapperNode* node) {
    visit(node->value);
}

void SymResLinkBody::VisitEmbeddedNode(EmbeddedNode* node) {
    node->sym_res_fn(&linker, node);
}

// --------------- Values and Types BEGIN HERE -----------------
// -------------------------------------------------------------
// -------------------------------------------------------------


void SymResLinkBody::VisitAccessChain(AccessChain *chain) {

    VisitAccessChain(chain, true, false);

}

void SymResLinkBody::VisitFunctionCall(FunctionCall* value) {
    // load the expected type beforehand
    const auto exp_type = expected_type;
    const auto parent_val = value->parent_val;
    visit(parent_val, nullptr);
    value->link_without_parent(linker, exp_type, true);
}

void SymResLinkBody::VisitNumberValue(NumberValue* value) {
    // DOES NOTHING AT THE MOMENT
}

void SymResLinkBody::VisitEmbeddedValue(EmbeddedValue* value) {
    value->sym_res_fn(&linker, value);
}

void SymResLinkBody::VisitComptimeValue(ComptimeValue* value) {
    visit(value->value, expected_type);
}

void SymResLinkBody::VisitIncDecValue(IncDecValue* value) {
    visit(value->value, expected_type);
}

void SymResLinkBody::VisitVariantCase(VariantCase* value) {
    // DOES NOTHING AT THE MOMENT
}

void SymResLinkBody::VisitArrayType(ArrayType* arrType) {
    auto& elem_type = arrType->elem_type;
    visit(elem_type);
    if(arrType->array_size_value) {
        visit(arrType->array_size_value);
        const auto evaluated = arrType->array_size_value->evaluated_value(linker.comptime_scope);
        const auto number = evaluated->get_the_number();
        if(number.has_value()) {
            arrType->set_array_size(number.value());
            return;
        }
        return;
    }
}

void SymResLinkBody::VisitDynamicType(DynamicType* type) {
    visit(type->referenced, type_location);
}

void link_param(SymResLinkBody& symRes, FunctionParam* param) {
    if(param->is_implicit()) {
        param->link_implicit_param(symRes.linker);
    } else {
        symRes.visit(param->type);
    }
}

void SymResLinkBody::VisitFunctionType(FunctionType* type) {
    for (auto &param : type->params) {
        link_param(*this, param);
    }
    visit(type->returnType);
    // TODO: cannot track this
    type->data.signature_resolved = true;
}

void SymResLinkBody::VisitGenericType(GenericType* gen_type) {
    visit(gen_type->referenced, type_location);
    for(auto& type : gen_type->types) {
        visit(type);
    }
    gen_type->instantiate(linker.genericInstantiator, type_location);
}

void SymResLinkBody::VisitLinkedType(LinkedType* type) {
    if(type->attrs.is_named) {
        const auto namedType = (NamedLinkedType*) type;
        auto link_name = namedType->debug_link_name();
        const auto found = linker.find(link_name);
        if(found) {
            type->linked = found;
        } else if(type->linked == nullptr) {
            linker.error(type_location) << "unresolved symbol, couldn't find referenced type '" << link_name << '\'';
            return;
        }
        return;
    } else if(type->attrs.is_value) {
        const auto value_type = (LinkedValueType*) type;
        const auto value = value_type->value;
        visit(value);
        // TODO: do not use linked node here
        const auto linked = value->linked_node();
        if(linked) {
            type->linked = linked;
        }
        return;
    }
}

void SymResLinkBody::VisitPointerType(PointerType* type) {
    visit(type->type, type_location);
}

void SymResLinkBody::VisitReferenceType(ReferenceType* type) {
    visit(type->type, type_location);
}

void SymResLinkBody::VisitCapturingFunctionType(CapturingFunctionType* type) {
    visit(type->func_type);
    visit(type->instance_type);
}

void SymResLinkBody::VisitStructType(StructType* type) {
    type->take_variables_from_parsed_nodes(linker);
    sym_res_vars_signature(linker, type);
    if(!type->name.empty()) {
        linker.declare(type->name, type);
    }
}

void SymResLinkBody::VisitUnionType(UnionType* type) {
    type->take_variables_from_parsed_nodes(linker);
    sym_res_vars_signature(linker, type);
    if(!type->name.empty()) {
        linker.declare(type->name, type);
    }
}

void SymResLinkBody::VisitAddrOfValue(AddrOfValue* addrOfValue) {
    const auto value = addrOfValue->value;
    visit(value);

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
    // TODO: remove this if link signature is not calling link function
    if(!linker.linking_signature) {
        // TODO we must perform mutability checks during link signature
        addrOfValue->is_value_mutable = value->check_is_mutable(linker.allocator, true);
    }
}

void SymResLinkBody::VisitArrayValue(ArrayValue* arrValue) {
    // load the expected type before visiting sub values
    const auto exp_type = expected_type;
    auto& values = arrValue->values;
    auto& elemType = arrValue->known_elem_type();
    if(elemType) {
        visit(elemType);
    }
    if(exp_type && exp_type->kind() == BaseTypeKind::Array) {
        const auto arr_type = (ArrayType*) exp_type;
        elemType = arr_type->elem_type.copy(*linker.ast_allocator);
    }
    if(elemType) {
        const auto def = elemType->linked_struct_def();
        if(def) {
            unsigned i = 0;
            while (i < values.size()) {
                auto& val_ptr = values[i];
                const auto value = val_ptr;
                visit(value, elemType);
                const auto implicit = def->implicit_constructor_func(linker.allocator, value);
                if(implicit) {
                    link_with_implicit_constructor(implicit, linker, value);
                } else if(!linker.linking_signature && !elemType->satisfies(linker.allocator, value, false)) {
                    linker.unsatisfied_type_err(value, elemType);
                }
                i++;
            }
            return;
        }
    }
    auto& current_func_type = *linker.current_func_type;
    auto& known_elem_type = elemType;
    unsigned i = 0;
    for(auto& value : values) {
        visit(value);
        if(i == 0 && !known_elem_type) {
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
}

void SymResLinkBody::VisitBlockValue(BlockValue* bValue) {
    auto& scope = bValue->scope;
    if(scope.nodes.empty()) {
        linker.error("empty block value not allowed", type_location);
        return;
    } else {
        scope.link_sequentially(linker);
    }
    const auto lastNode = scope.nodes.back();
    const auto lastKind = lastNode->kind();
    if(lastKind != ASTNodeKind::ValueWrapper) {
        linker.error("block doesn't contain a value wrapper as last node", type_location);
        return;
    }
    const auto lastValNode = lastNode->as_value_wrapper_unsafe();
    bValue->calculated_value = lastValNode->value;
    return;
}

void SymResLinkBody::VisitCastedValue(CastedValue* cValue) {
    auto& type = cValue->type;
    const auto value = cValue->value;
    visit(type);
    visit(value, type);
}

void SymResLinkBody::VisitDereferenceValue(DereferenceValue* value) {
    if(linker.safe_context) {
        linker.warn("dereferencing a pointer in safe context is prohibited", value);
    }
    visit(value->value);
}

void SymResLinkBody::VisitExpression(Expression* value) {
    visit(value->firstValue);
    visit(value->secondValue);
    // ast allocator is being used
    // it's unknown when this expression should be disposed
    // file level / module level allocator should be used, when this expression belongs to a function
    // or decl that is private or internal, however that is hard to determine
    // TODO: remove this check
    if(!linker.linking_signature) {
        // TODO this created_type should always be created, however this creates an error
        value->created_type = value->create_type(*linker.ast_allocator);
    }
}

void SymResLinkBody::VisitIndexOperator(IndexOperator* indexOp) {
    auto& values = indexOp->values;
    visit(indexOp->parent_val);
    for(auto& value : values) {
        visit(value);
    }
}

void SymResLinkBody::VisitIsValue(IsValue* isValue) {
    const auto value = isValue->value;
    auto& type = isValue->type;
    visit(value);
    visit(type);
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
            link_param(temp_linker, param);
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

void copy_func_params_types(const std::vector<FunctionParam*>& from_params, std::vector<FunctionParam*>& to_params, SymResLinkBody& symRes, Value* debug_value) {
    auto& resolver = symRes.linker;
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
            link_param(symRes, to_param);
            // check it's type is same as the from parameter
            if(!to_param->type->is_same(from_param->type)) {
                resolver.error(debug_value) << "Lambda function param at index " << std::to_string(start) << " with type " << from_param->type->representation() << ", redeclared with type " << to_param->type->representation();
            }
        }
        start++;
    }
}

bool link_lambda(LambdaFunction* func, SymResLinkBody& symRes ,FunctionType* func_type) {
    auto& linker = symRes.linker;
    auto& params = func->params;
    auto& returnType = func->returnType;
    copy_func_params_types(func_type->params, params, symRes, func);
    if(!returnType) {
        returnType = func_type->returnType.copy(*linker.ast_allocator);
    } else if(!returnType->is_same(func_type->returnType)) {
        linker.error((Value*) func) << "Lambda function type expected return type to be " << func_type->returnType->representation() << " but got lambda with return type " << returnType->representation();
    }
    func->setIsCapturing(func_type->isCapturing());
    return true;
}

void SymResLinkBody::VisitLambdaFunction(LambdaFunction* lambVal) {

    auto& scope = lambVal->scope;
    auto& returnType = lambVal->returnType;
    auto& data = lambVal->data;
    auto& params = lambVal->params;
    auto& captureList = lambVal->captureList;

    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = lambVal;

    auto func_type = expected_type ? expected_type->canonical()->get_function_type() : nullptr;

    if(!func_type) {

#ifdef DEBUG
        linker.info("deducing lambda function type by visiting body", (Value*) lambVal);
#endif

        // linking params and their types
        auto result = link_full(lambVal, linker, true);

        // finding return type
        auto found_return_type = find_return_type(*linker.ast_allocator, scope.nodes);

        returnType = {found_return_type, lambVal->get_location()};

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

        link_lambda(lambVal, *this, func_type);

        if(link_full(lambVal, linker, false)) {
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

        lambVal->setIsCapturing(true);

    }

    linker.current_func_type = prev_func_type;

    return;

}

void SymResLinkBody::VisitNegativeValue(NegativeValue* negValue) {
    visit(negValue->value);
}

void SymResLinkBody::VisitUnsafeValue(UnsafeValue* value) {
    const auto prev = linker.safe_context;
    linker.safe_context = false;
    visit(value->value, expected_type);
    linker.safe_context = prev;
}

void SymResLinkBody::VisitNewValue(NewValue* value) {
    visit(value->value);
}

void SymResLinkBody::VisitTypeInsideValue(TypeInsideValue* value) {
    visit(value->type, value->encoded_location());
}

void SymResLinkBody::VisitNewTypedValue(NewTypedValue* value) {
    visit(value->type);
}

void SymResLinkBody::VisitPlacementNewValue(PlacementNewValue* value) {
    visit(value->pointer);
    visit(value->value);
}

void SymResLinkBody::VisitNotValue(NotValue* value) {
    visit(value->value);
}

void SymResLinkBody::VisitNullValue(NullValue* value) {
    if(expected_type) {
        const auto kind = expected_type->kind();
        if(kind == BaseTypeKind::Function || kind == BaseTypeKind::Pointer) {
            value->expected = expected_type->copy(*linker.ast_allocator);
        }
    }
}

void SymResLinkBody::VisitPatternMatchExpr(PatternMatchExpr* expr) {
    const auto expression = expr->expression;
    auto& elseExpression = expr->elseExpression;
    auto& param_names = expr->param_names;
    visit(expression);
    const auto child_member = expr->find_member_from_expr(linker.allocator, linker);
    if(!child_member) {
        return;
    }
    // set the member, so we don't need to resolve it again
    expr->member = child_member;
    auto& params = child_member->values;
    if(elseExpression.kind == PatternElseExprKind::DefValue && param_names.size() != 1) {
        linker.error("must destructure one member for default value to work", expr);
        return;
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
    if (elseVal) {
        visit(elseVal);
        return;
    }
}

void SymResLinkBody::VisitSizeOfValue(SizeOfValue* value) {
    visit(value->for_type);
}

void SymResLinkBody::VisitAlignOfValue(AlignOfValue* value) {
    visit(value->for_type);
}

void SymResLinkBody::VisitStringValue(StringValue* strValue) {
    const auto type = expected_type;
    if (type && type->kind() == BaseTypeKind::Array) {
        strValue->is_array = true;
        auto arrayType = (ArrayType*) (type);
        if (arrayType->get_array_size() > strValue->value.size()) {
            strValue->length = (unsigned int) arrayType->get_array_size();
        } else if (arrayType->has_no_array_size()) {
            strValue->length = strValue->value.size() + 1; // adding 1 for the last /0
        } else {
#ifdef DEBUG
            throw std::runtime_error("unknown");
#endif
        }
    }
}

void SymResLinkBody::VisitStructValue(StructValue* structValue) {
    // load the expected type before hand
    const auto exp_type = expected_type;
    if(structValue->refType) {
        visit(structValue->refType);
    } else {
        if(!exp_type) {
            linker.error("unnamed struct value cannot link without a type", structValue);
            structValue->refType = { new (linker.ast_allocator->allocate<StructType>()) StructType("", nullptr, structValue->encoded_location()), structValue->encoded_location()};
            return;
        }
        structValue->refType = {exp_type, structValue->refType.getLocation()};
    }
    if(!structValue->resolve_container(linker.genericInstantiator)) {
        return;
    }
    structValue->diagnose_missing_members_for_init(linker);
    if(!structValue->allows_direct_init()) {
        linker.error(structValue) << "struct value with a constructor cannot be initialized, name '" << structValue->linked_extendable()->name_view() << "' has a constructor";
    }
    auto refTypeKind = structValue->refType->kind();
    if(refTypeKind == BaseTypeKind::Generic) {
        for (auto& arg: structValue->generic_list()) {
            visit(arg);
        }
    }
    auto& current_func_type = *linker.current_func_type;
    // linking values
    for (auto &val: structValue->values) {
        auto& val_ptr = val.second.value;
        const auto value = val_ptr;
        auto child_node = structValue->variables()->child_member_or_inherited_struct(val.first);
        if(!child_node) {
            linker.error(structValue) << "unresolved child '" << val.first << "' in struct declaration";
            continue;
        }
        auto child_type = child_node->known_type();
        visit(val_ptr, child_type);
        const auto member = structValue->variables()->direct_variable(val.first);
        if(member) {
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
}

void SymResLinkBody::VisitVariableIdentifier(VariableIdentifier* value) {
    // by default access is checked
    // TODO: make this method inline
    VisitVariableIdentifier(value, true);
}