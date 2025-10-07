// Copyright (c) Chemical Language Foundation 2025.

#include "ast/statements/Assignment.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/Break.h"
#include "ast/statements/DestructStmt.h"
#include "ast/statements/DeallocStmt.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/Return.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/statements/UnresolvedDecl.h"
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
#include "ast/base/TypeBuilder.h"
#include "ast/types/VoidType.h"
#include "ast/values/NullValue.h"
#include "ast/values/StringValue.h"
#include "ast/types/LinkedValueType.h"
#include "compiler/symres/SymbolResolver.h"
#include "ast/utils/ASTUtils.h"
#include "SymResLinkBody.h"
#include "SymResLinkBodyAPI.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "ast/utils/GenericUtils.h"
#include "NodeSymbolDeclarer.h"
#include "ast/base/ChildResolution.h"
#include "preprocess/visitors/RecursiveVisitor.h"

void sym_res_link_body(SymbolResolver& resolver, Scope* scope) {
    SymResLinkBody linker(resolver);
    linker.VisitScope(scope);
}

/**
 * when nodes are to be declared and used sequentially, so node can be referenced
 * after it is declared, this method should be called
 * for example, this code, i should be declared first then incremented
 * var i = 0;
 * i++; <--- i is declared above (if it's below it shouldn't be referencable)
 */
void link_seq(SymResLinkBody& visitor, Scope& scope) {
    auto& nodes = scope.nodes;
    if(nodes.empty()) return;
    const auto curr_func = visitor.linker.current_func_type;
    if(curr_func) {
        const auto moved_ids_begin = curr_func->moved_identifiers.size();
        const auto moved_chains_begin = curr_func->moved_chains.size();
        visitor.VisitScope(&scope);
        if (nodes.back()->kind() == ASTNodeKind::ReturnStmt) {
            curr_func->erase_moved_ids_after(moved_ids_begin);
            curr_func->erase_moved_chains_after(moved_chains_begin);
        }
    } else {
        visitor.VisitScope(&scope);
    }
}

/**
 * this will link the given body sequentially, backing the moved identifiers and chains
 * into the given vectors, which you can restore later
 */
void link_seq_backing_moves(
        SymResLinkBody& visitor,
        Scope& scope,
        std::vector<VariableIdentifier*>& moved_ids,
        std::vector<AccessChain*>& moved_chains
) {
    const auto curr_func = visitor.linker.current_func_type;
    if(!curr_func) return;

    // where the moved ids / chains of if body begin
    const auto if_moved_ids_begin = curr_func->moved_identifiers.size();
    const auto if_moved_chains_begin = curr_func->moved_chains.size();

    // link the body
    link_seq(visitor, scope);

    // save all the moved identifiers / chains inside the if body to temporary location
    curr_func->save_moved_ids_after(moved_ids, if_moved_ids_begin);
    curr_func->save_moved_chains_after(moved_chains, if_moved_chains_begin);
}

void MembersContainer::declare_inherited_members(SymbolResolver& linker) {
    const auto container = this;
    for(const auto var : container->variables()) {
        linker.declare(var->name, var);
    }
    for (const auto func: container->functions()) {
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
    for(auto& inherits : container->inherited) {
        const auto def = inherits.type->get_members_container();
        if(def) {
            if(def == container) {
                linker.error(inherits.type.encoded_location()) << "recursion in inheritance is not allowed";
                continue;
            }
            def->declare_inherited_members(linker);
        }
    }
}

void MembersContainer::redeclare_inherited_members(SymbolResolver &linker) {
    for(auto& inherits : inherited) {
        const auto def = inherits.type->get_members_container();
        if(def) {
            def->declare_inherited_members(linker);
        }
    }
}

void MembersContainer::redeclare_variables_and_functions(SymbolResolver &linker) {
    for (const auto var: variables()) {
        linker.declare(var->name, var);
    }
    for(const auto func : functions()) {
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
    SymbolResolverDeclarer declarer(linker);
    // declare all the functions
    for(auto& func : container->functions()) {
        declare_node(declarer, func, AccessSpecifier::Private);
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

bool find_link_in_parent(VariableIdentifier* id, ChainValue* parent, SymbolResolver& resolver) {
    auto& value = id->value;
    const auto child = provide_child(&resolver.child_resolver, parent, value, nullptr);
    if(child) {
        id->linked = child;
        id->setType(child->known_type());
        id->process_linked(&resolver, resolver.current_func_type);
        return true;
    } else {
        id->linked = resolver.get_unresolved_decl();
        id->setType(id->linked->known_type());
        resolver.error(id) << "unresolved child '" << value << "' in parent '" << parent->representation() << "'";
        return false;
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
            auto self_param = linker.current_func_type->get_self_param();
            if (!self_param) {
                auto decl = linker.current_func_type->as_function();
                if (!decl || !decl->is_constructor_fn() && !decl->is_comptime()) {
                    linker.error(values[0]) << "unresolved identifier '" << values[0]->representation() << "', because function doesn't take a self argument";
                }
            }
        }
    }

    if(values.size() == 1) {
        chain->setType(values[0]->getType());
        return;
    }

    const auto values_size = values.size();
    const auto last = values_size - 1;
    unsigned i = 1;
    while (i < values_size) {
        find_link_in_parent(values[i]->as_identifier_unsafe(), values[i - 1], linker);
        i++;
    }

    // the last item holds the type for this access chain
    chain->setType(values[last]->getType());

    if(check_validity && linker.current_func_type) {
        // check chain for validity, if it's moved or members have been moved
        linker.current_func_type->check_chain(chain, assignment, linker);
    }
}

void SymResLinkBody::VisitVariableIdentifier(VariableIdentifier* identifier, bool check_access) {
    auto& value = identifier->value;
    const auto linked = linker.find(value);
    if(linked) {
        identifier->linked = linked;
        identifier->setType(linked->known_type());
        if (check_access && linker.current_func_type) {
            // check for validity if accessible or assignable (because moved)
            linker.current_func_type->check_id(identifier, linker);
        }
        identifier->process_linked(&linker, linker.current_func_type);
        return;
    } else {
        // since we couldn't find a linked declaration, we will
        // link this identifier with unresolved declaration
        identifier->linked = linker.get_unresolved_decl();
        identifier->setType(identifier->linked->known_type());
        linker.error(identifier) << "unresolved variable identifier '" << value << "' not found";
    }
}

bool is_assignable(Value* lhs) {
    switch(lhs->kind()) {
        case ValueKind::AddrOfValue:
        case ValueKind::IsValue:
        case ValueKind::StructValue:
        case ValueKind::ArrayValue:
        case ValueKind::LambdaFunc:
        case ValueKind::SizeOfValue:
        case ValueKind::AlignOfValue:
        case ValueKind::String:
        case ValueKind::IntN:
        case ValueKind::IfValue:
        case ValueKind::SwitchValue:
        case ValueKind::LoopValue:
        case ValueKind::VariantCase:
        case ValueKind::Bool:
        case ValueKind::Float:
        case ValueKind::Double:
        case ValueKind::NullValue:
        case ValueKind::NotValue:
        case ValueKind::NegativeValue:
        case ValueKind::NewValue:
        case ValueKind::NewTypedValue:
        case ValueKind::PlacementNewValue:
        case ValueKind::ComptimeValue:
        case ValueKind::CastedValue:
        case ValueKind::UnsafeValue:
        case ValueKind::Expression:
        case ValueKind::IncDecValue:
            return false;
        case ValueKind::AccessChain:
            return is_assignable(lhs->as_access_chain_unsafe()->values.back());
        case ValueKind::FunctionCall:
            return lhs->getType()->isReferenceCanonical();
        default:
            return true;
    }
}

void SymResLinkBody::VisitAssignmentStmt(AssignStatement *assign) {
    auto& lhs = assign->lhs;
    auto& value = assign->value;

    link_assignable(*this, lhs, nullptr);

    if(!is_assignable(lhs)) {
        linker.error("Expression is not assignable", lhs);
    }

    const auto lhsType = lhs->getType();

    visit(value, lhsType);

    // first we check if the value is mutable
    // immutable values cannot be used (even in operator overloading)
    if(!lhs->check_is_mutable(linker.allocator, true)) {
        linker.error("cannot assign to a non mutable value", lhs);
    }

    // check if operator is overloaded
    // direct assignment cannot be overloaded
    if(assign->assOp != Operation::Assignment) {
        const auto can_node = lhsType->get_linked_canonical_node(true, false);
        if(can_node) {
            const auto container = can_node->get_members_container();
            if(container) {
                auto func_name = AssignStatement::overload_op_name(assign->assOp);
                if(func_name.empty()) {
                    linker.error("cannot override this operator", assign);
                    return;
                }
                const auto child = container->child(func_name);
                if(!child) {
                    linker.error(assign) << "expected a function with name '" << func_name << "' for operator implementation";
                    return;
                }
                auto& func_type = *linker.current_func_type;
                if(child->kind() == ASTNodeKind::FunctionDecl) {
                    const auto func = child->as_function_unsafe();
                    if(func->params.size() != 2) {
                        linker.error("expected overload implementation to have exactly two parameters", assign);
                        return;
                    }
                    // check if rhs was moved and mark it
                    func_type.mark_moved_value(linker.allocator, value, func->params[1]->known_type(), linker, true);
                } else if(child->kind() == ASTNodeKind::MultiFunctionNode) {
                    const auto node = child->as_multi_func_node_unsafe();
                    std::vector<Value*> args { assign->lhs, assign->value };
                    const auto func = node->func_for_call(args);
                    if(!func) {
                        linker.error(assign) << "couldn't find function with name '" << func_name << "' for operator implementation that satisfies these arguments";
                        return;
                    }
                    // check if rhs was moved and mark it
                    func_type.mark_moved_value(linker.allocator, value, func->params[1]->known_type(), linker, true);
                } else {
                    linker.error(assign) << "expected a function with name '" << func_name << "' for operator implementation";
                    return;
                }
                return;
            }
        }
    }

    // check assignment satisfies the lhs type
    switch(assign->assOp){
        case Operation::Assignment:
            if (!lhsType->satisfies(value, true)) {
                linker.unsatisfied_type_err(value, lhsType);
            }
            break;
        case Operation::Addition:
        case Operation::Subtraction:
            if(lhsType->kind() == BaseTypeKind::Pointer) {
                const auto rhsType = value->getType()->canonical();
                if(rhsType->kind() != BaseTypeKind::IntN) {
                    linker.unsatisfied_type_err(value, lhsType);
                }
            } else if (!lhsType->satisfies(value, true)) {
                linker.unsatisfied_type_err(value, lhsType);
            }
            break;
        default:
            break;
    }

    // we should report has assignment
    // even if it's a different operator
    // so the parameter can be allocated in a temporary variable for modification
    auto id = lhs->get_single_id();
    if(id) {
        auto linked = id->linked;
        auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::VarInitStmt) {
            auto init = linked->as_var_init_unsafe();
            init->set_has_assignment();
        } else if(linked_kind == ASTNodeKind::FunctionParam) {
            auto param = linked->as_func_param_unsafe();
            param->set_has_assignment();
        }
    }

    // if overloaded operator is being called, lhs will not be un-moved
    // because all operator overloaded take self (lhs) as mut ref
    auto& func_type = *linker.current_func_type;
    func_type.mark_un_moved_lhs_value(lhs, lhs->getType());
    // check if rhs was moved and mark it
    func_type.mark_moved_value(linker.allocator, value, lhs->getType(), linker, true);

}

void UsingStmt::declare_symbols(SymbolResolver &linker) {
    auto linked = chain->linked_node();
    if(!linked) {
        linker.error("couldn't find linked node", this);
        return;
    }
    if(is_namespace()) {
        auto ns = linked->as_namespace();
        if(ns) {
            for(auto& node_pair : ns->extended) {
                const auto node = node_pair.second;
                linker.declare(chem::string_view(node_pair.first.data(), node_pair.first.size()), node);
            }
        } else {
            linker.error("expected value to be a namespace, however it isn't", this);
        }
    } else {
        const auto& name_view = linked->get_located_id()->identifier;
        linker.declare(name_view, linked);
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
    auto type = identifier->getType()->canonical();
    if(!type->is_pointer()) {
        linker.error("destruct cannot be called on a value that isn't a pointer", node);
        return;
    }
}

void SymResLinkBody::VisitDeallocStmt(DeallocStmt* node) {
    visit(node->ptr);
}

void SymResLinkBody::VisitProvideStmt(ProvideStmt* node) {
    auto& value = node->value;
    visit(value);
    node->put_in(linker.implicit_args, value, this, [](ProvideStmt* stmt, void* data) {
        link_seq((*(SymResLinkBody*) data), stmt->body);
    });
}

bool link_call_without_parent(SymResLinkBody& visitor, FunctionCall* call, BaseType* expected_type, bool link_implicit_constructor);

void link_with_implicit_constructor(SymResLinkBody& visitor, FunctionDeclaration* decl, Value* value) {
    VariableIdentifier id(decl->name_view(), ZERO_LOC);
    id.linked = decl;
    id.setType(decl->known_type());
    FunctionCall imp_call(&id, ZERO_LOC);
    imp_call.values.emplace_back(value);
    link_call_without_parent(visitor, &imp_call, nullptr, false);
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
            const auto implicit = func_type->returnType->implicit_constructor_for(value);
            if (implicit &&
                // this check means current function is not the implicit constructor we're trying to link for value
                // basically an implicit constructor can has a value returned of a type for which it's an implicit constructor of (in comptime)
                implicit != func_type &&
                // this check means current function's parent (if it's inside a struct) is not the same parent as the implicit constructor parent
                // meaning implicit constructor and the function that's going to use the implicit constructor can't be inside same container
                (func && func->parent() != implicit->parent())
            ) {
                link_with_implicit_constructor(*this, implicit, value);
                return;
            }
            if(!func_type->returnType->satisfies(value, false)) {
                linker.unsatisfied_type_err(value, func_type->returnType);
            }
        }
    }
}

VariantCase* create_variant_case(SymbolResolver& resolver, SwitchStatement* stmt, VariantDefinition* def, VariableIdentifier* id) {
    auto& allocator = *resolver.ast_allocator;
    // explicit nullptr for ChildResolver, because we're just looking for direct variant members
    const auto child = def->child(nullptr, id->value);
    if(child) {
        if(child->kind() == ASTNodeKind::VariantMember) {
            return new (allocator.allocate<VariantCase>()) VariantCase(child->as_variant_member_unsafe(), stmt, resolver.comptime_scope.typeBuilder.getVoidType(), id->encoded_location());
        } else {
            resolver.error(id) << "couldn't find variant member with name '" << id->value << "'";
        }
    } else {
        resolver.error(id) << "couldn't find the variant member with name '" << id->value << "'";
    }
    return nullptr;
}

void create_var_case_var(VariableIdentifier* id, SymResLinkBody& linker, ASTAllocator& allocator, VariantCase* varCase, SwitchStatement* stmt) {
    const auto param = varCase->member->values.find(id->value);
    if (param != varCase->member->values.end()) {

        auto variable = new(allocator.allocate<VariantCaseVariable>()) VariantCaseVariable(id->value, param->second, stmt, 0);
        varCase->identifier_list.emplace_back(variable);
        linker.visit(variable);

    } else {
        linker.linker.error("couldn't find variant member parameter with that name", id);
    }
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
                    create_var_case_var(value->as_identifier_unsafe(), linker, astAlloc, varCase, stmt);
                } else if(value->kind() == ValueKind::AccessChain && value->as_access_chain_unsafe()->values.back()->kind() == ValueKind::Identifier) {
                    create_var_case_var(value->as_access_chain_unsafe()->values.back()->as_identifier_unsafe(), linker, astAlloc, varCase, stmt);
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
        link_seq_backing_moves(*this, scope, moved_ids, moved_chains);
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
    if(node->is_top_level()) {
        return;
    }
    auto& type = node->type;
    auto& value = node->value;
    auto& attrs = node->attrs;
    if(type) {
        visit(type);
    }
    if(value) {
        visit(value, node->type_ptr_fast());
    }
    const auto type_resolved = true;
    const auto value_resolved = true;
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
            if(!type->satisfies(value, false)) {
                linker.unsatisfied_type_err(value, type);
            }
        }
    }
}

void SymResLinkBody::VisitComptimeBlock(ComptimeBlock* node) {
    link_seq(*this, node->body);
}

void SymResLinkBody::VisitDoWhileLoopStmt(DoWhileLoop* node) {
    linker.scope_start();
    link_seq(*this, node->body);
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
    link_seq(*this, node->body);
    linker.scope_end();
}

void SymResLinkBody::VisitFunctionParam(FunctionParam* node) {
    linker.declare(node->name, node);
}

void SymResLinkBody::VisitGenericTypeParam(GenericTypeParameter* node) {
    if(node->at_least_type) {
        visit(node->at_least_type);
    }
    linker.declare(node->identifier, node);
    if(node->def_type) {
        visit(node->def_type);
    }
}

void set_implicit_self_linked_to(FunctionParam* param, BaseType* linked_type) {
    if (param->type->kind() == BaseTypeKind::Reference) {
        param->type->as_reference_type_unsafe()->type = linked_type;
    } else {
        param->type = TypeLoc(linked_type, param->type.encoded_location());
    }
}

void set_implicit_self_linked_to(ASTAllocator& allocator, FunctionParam* param, ASTNode* linked_node) {
    const auto linked_type = new (allocator.allocate<LinkedType>()) LinkedType(linked_node, false, false);
    if (param->type->kind() == BaseTypeKind::Reference) {
        param->type->as_reference_type_unsafe()->type = linked_type;
    } else {
        param->type = TypeLoc(linked_type, param->type.encoded_location());
    }
}

bool link_self_type(SymbolResolver& linker, FunctionParam* param) {
    auto parent_node = param->parent();
    auto parent_kind = parent_node->kind();
    if (parent_kind == ASTNodeKind::FunctionDecl || parent_kind == ASTNodeKind::StructMember) {
        const auto p = parent_node->parent();
        if (p) {
            parent_node = p;
            parent_kind = p->kind();
        }
    }
    switch (parent_kind) {
        case ASTNodeKind::ImplDecl: {
            const auto struct_type = parent_node->as_impl_def_unsafe()->struct_type;
            if(struct_type) {
                set_implicit_self_linked_to(param, struct_type);
                return true;
            } else {
                linker.error("couldn't get self / other implicit parameter type", param);
                return false;
            }
        }
        case ASTNodeKind::GenericImplDecl: {
            const auto struct_type = parent_node->as_gen_impl_decl_unsafe()->master_impl->struct_type;
            if(struct_type) {
                set_implicit_self_linked_to(param, struct_type);
                return true;
            } else {
                linker.error("couldn't get self / other implicit parameter type", param);
                return false;
            }
        }
        case ASTNodeKind::GenericInterfaceDecl:
        case ASTNodeKind::GenericUnionDecl:
        case ASTNodeKind::GenericStructDecl:
        case ASTNodeKind::GenericVariantDecl:
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::InterfaceDecl:
            set_implicit_self_linked_to(*linker.ast_allocator, param, parent_node);
            return true;
        default:
            linker.error("couldn't get self / other implicit parameter type", param);
            return false;
    }
}

bool FunctionParam::link_implicit_param(SymbolResolver& linker) {
    // TODO: remove other from here
    if(name == "self" || name == "other") { // name and other means pointers to parent node
        return link_self_type(linker, this);
    } else {
        auto found = linker.find(name);
        if(found) {
            const auto ptr_type = ((ReferenceType*) type.getType());
            const auto linked_type = ((LinkedType*) ptr_type->type);
            const auto found_kind = found->kind();
            if(found_kind == ASTNodeKind::TypealiasStmt) {
                const auto retrieved = ((TypealiasStatement*) found)->actual_type;
                type = { retrieved, type.encoded_location() };
                const auto direct = retrieved->get_direct_linked_node();
                if(direct && ASTNode::isStoredStructType(direct->kind())) {
                    linker.error("struct like types must be passed as references using implicit parameters with typealias, please add '&' to make it a reference", this);
                    return false;
                }
            } else {
                linked_type->linked = found;
            }
        } else {
            linker.error("couldn't get implicit parameter type", this);
            return false;
        }
        return true;
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
            link_seq(*this, node->body.value());
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
    } else {
        linker.error(node) << "unresolved identifier '" << node->name << "' captured";
        node->linked = linker.get_unresolved_decl();
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
    link_seq_backing_moves(symRes, body, moved_ids, moved_chains);
    linker.scope_end();
}

void link_conditions(IfStatement* stmt, SymResLinkBody& symRes) {
    symRes.visit(stmt->condition);
    for (auto& cond: stmt->elseIfs) {
        symRes.visit(cond.first);
    }
}

void IfStatement::link_conditions(SymbolResolver &linker) {
    SymResLinkBody temp(linker);
    ::link_conditions(this, temp);
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

Scope* IfStatement::link_evaluated_scope(SymbolResolver& linker) {
    if(computed_scope.has_value()) {
        return computed_scope.value();
    }
    link_conditions(linker);
    auto condition_val = get_condition_const(linker.comptime_scope);
    if(condition_val.has_value()) {
        auto eval = get_evaluated_scope(linker.comptime_scope, &linker, condition_val.value());
        computed_scope = eval;
        return eval;
    } else {
        linker.error("couldn't evaluate if statement at compile time", this);
        return nullptr;
    }
}

void SymResLinkBody::VisitIfStmt(IfStatement* node) {

    link_conditions_no_patt_match_expr(node, *this);

    // evaluate the scope and only link that scope
    if(node->is_comptime()) {
        auto condition_val = node->get_condition_const(linker.comptime_scope);
        if (condition_val.has_value()) {
            auto eval = node->get_evaluated_scope(linker.comptime_scope, &linker, condition_val.value());
            // computed scope is calculated once in sym res link body
            node->computed_scope = eval;
            if (eval) {
                visit(eval);
            }
            return;
        } else {
            linker.error("couldn't evaluate if statement at compile time", node);
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
        link_seq_backing_moves(*this, node->elseBody.value(), moved_ids, moved_chains);
        linker.scope_end();
    }

    curr_func->restore_moved_ids(moved_ids);
    curr_func->restore_moved_chains(moved_chains);

}

void SymResLinkBody::VisitImplDecl(ImplDefinition* node) {
    const auto linked_node = node->interface_type->get_direct_linked_canonical_node();
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

        // make struct adopt all the methods of interface
        // this should be done when the struct has been linked
        // if its body is being linked, we don't want to call the methods in interface
        if(!overrides_interface) {
            struct_linked->adopt(linked);
        }
    }
    LinkMembersContainerNoScope(node);
    linker.scope_end();
}

void SymResLinkBody::VisitNamespaceDecl(Namespace* node) {
    linker.scope_start();
    if(node->root) {
        node->root->declare_extended_in_linker(linker);
    } else {
        SymbolResolverDeclarer declarer(linker);
        for(const auto child : node->nodes) {
            declare_node(declarer, child, AccessSpecifier::Private);
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
    link_seq(*this, node->body);

}

void SymResLinkBody::VisitInitBlock(InitBlock* node) {
    const auto p = node->parent();
    if(!(p->kind() == ASTNodeKind::FunctionDecl && p->as_function_unsafe()->is_constructor_fn())) {
        linker.error("unexpected init block, must be present in a function only", node);
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
    link_seq(*this, node->scope);
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
    link_seq(*this, node->body);
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

void SymResLinkBody::VisitAccessChainNode(AccessChainNode* node) {
    visit(&node->chain);
}

void SymResLinkBody::VisitIncDecNode(IncDecNode* node) {
    visit(&node->value);
}

void SymResLinkBody::VisitPatternMatchExprNode(PatternMatchExprNode* node) {
    visit(&node->value);
}

void SymResLinkBody::VisitPlacementNewNode(PlacementNewNode* node) {
    visit(&node->value);
}

bool embedded_traverse(void* data, ASTAny* item) {
    const auto traverser = static_cast<SymResLinkBody*>(data);
    switch(item->any_kind()) {
        case ASTAnyKind::Node:
            traverser->visit(((ASTNode*) item));
            break;
        case ASTAnyKind::Value:
            traverser->visit(((Value*) item));
            break;
        case ASTAnyKind::Type:
            traverser->visit(((BaseType*) item), ZERO_LOC);
            break;
    }
    return true;
}

void SymResLinkBody::VisitEmbeddedNode(EmbeddedNode* node) {
    auto traverser = linker.binder.findHook(node->name, CBIFunctionType::TraversalNode);
    if(traverser) {
        // traverse any nested values and set their types
        ((EmbeddedNodeTraversalFunc) traverser)(node, this, embedded_traverse);
    } else {
        linker.error(node) << "couldn't find traversal method of embedded node with name '" << node->name << "' for symbol resolution";
    }
    auto found = linker.binder.findHook(node->name, CBIFunctionType::SymResNode);
    if(found) {
        ((EmbeddedNodeSymbolResolveFunc) found)(&linker, node);
    } else {
        linker.error(node) << "couldn't find symbol resolve method for embedded node with name '" << node->name << "'";
    }
}

// --------------- Values and Types BEGIN HERE -----------------
// -------------------------------------------------------------
// -------------------------------------------------------------


void SymResLinkBody::VisitAccessChain(AccessChain *chain) {

    VisitAccessChain(chain, true, false);

}

void link_call_values(SymResLinkBody& visitor, FunctionCall* call) {

    const auto parent_val = call->parent_val;
    auto& values = call->values;
    auto& linker = visitor.linker;

    auto& current_func = *linker.current_func_type;
    const auto parent = parent_val->linked_node();
    if(parent) {
        const auto variant_mem = parent->as_variant_member();
        if (variant_mem) {
            unsigned i = 0;
            const auto values_size = values.size();
            const auto total_params = variant_mem->values.size();
            while (i < values_size) {
                auto& value_ptr = values[i];
                auto& value = *value_ptr;
                const auto param = i < total_params ? (variant_mem->values.begin() + i)->second : nullptr;
                const auto expected_type = param ? param->type : nullptr;
                visitor.visit(&value, expected_type);
                current_func.mark_moved_value(linker.allocator, &value, expected_type, linker);
                i++;
            }

            // checking arguments exist for all variant call parameters
            const auto func_param_size = variant_mem->values.size();
            while (i < func_param_size) {
                auto param = (variant_mem->values.begin() + i)->second;
                if (param) {
                    linker.error(call) << "variant call parameter '" << param->name << "' doesn't have a default value and no argument exists for it";
                } else {
#ifdef DEBUG
                    throw std::runtime_error("couldn't get param");
#endif
                }
                i++;
            }

            return;
        }
    }

    auto func_type = call->function_type();
    if (func_type && !func_type->data.signature_resolved) {
        linker.error("calling a function whose signature couldn't be resolved", call);
        return;
    }

    unsigned i = 0;
    const auto values_size = values.size();
    while (i < values_size) {
        auto& value_ptr = values[i];
        auto& value = *value_ptr;
        const auto param = func_type ? func_type->func_param_for_arg_at(i) : nullptr;
        const auto expected_type = param ? param->type : nullptr;
        visitor.visit(&value, expected_type);
        current_func.mark_moved_value(linker.allocator, &value, expected_type, linker);
        i++;
    }

    // checking arguments exist for all function call values
    if(func_type) {
        const auto func_param_size = func_type->expectedArgsSize();
        while (i < func_param_size) {
            auto param = func_type->func_param_for_arg_at(i);
            if (param) {
                if (param->defValue == nullptr && !func_type->isInVarArgs(i)) {
                    linker.error(call) << "function parameter '" << param->name << "' doesn't have a default value and no argument exists for it";
                }
            } else {
#ifdef DEBUG
                throw std::runtime_error("couldn't get param");
#endif
            }
            i++;
        }
    }

}

void link_call_args_implicit_constructor(SymResLinkBody& visitor, FunctionCall* call){
    auto& linker = visitor.linker;
    auto func_type = call->function_type();
    if(!func_type || !func_type->data.signature_resolved) return;
    unsigned i = 0;
    while(i < call->values.size()) {
        const auto param = func_type->func_param_for_arg_at(i);
        if (param) {
            const auto value = call->values[i];
            auto implicit_constructor = param->type->implicit_constructor_for(value);
            if (implicit_constructor) {
                link_with_implicit_constructor(visitor, implicit_constructor, value);
            } else if(!param->type->satisfies(value, false)) {
                linker.unsatisfied_type_err(value, param->type);
            }
        }
        i++;
    }
}

bool link_call_gen_args(SymResLinkBody& visitor, FunctionCall* call) {
    auto& linker = visitor.linker;
    for(auto& type : call->generic_list) {
        visitor.visit(const_cast<BaseType*>(type.getType()), type.getLocation());
    }
    return true;
}

void update_parent_val_linked(ChainValue* value, ASTNode* node, BaseType* type) {
    switch(value->kind()) {
        case ValueKind::Identifier:
            value->as_identifier_unsafe()->linked = node;
            value->as_identifier_unsafe()->setType(type);
            return;
        case ValueKind::AccessChain: {
            const auto last = value->as_access_chain_unsafe()->values.back();
#ifdef DEBUG
            if(last->kind() != ValueKind::Identifier) {
                throw std::runtime_error("this should always be an id");
            }
#endif
            const auto id = last->as_identifier_unsafe();
            id->linked = node;
            id->setType(type);
            value->setType(type);
            return;
        }
        default:
            return;
    }
}

Value* getNonComptimeValue(BaseType* type, Value* value);

bool isAnyRuntimeType(BaseType* type);

Value* getNonComptimeValueByKind(BaseType* type, Value* value) {
    switch(value->kind()) {
        case ValueKind::Identifier: {
            const auto linked = value->as_identifier_unsafe()->linked;
            switch (linked->kind()) {
                case ASTNodeKind::EnumMember:
                    return nullptr;
                case ASTNodeKind::FunctionParam: {
                    const auto p = linked->as_func_param_unsafe()->parent();
                    if(p->kind() == ASTNodeKind::FunctionDecl) {
                        return p->as_function_unsafe()->is_comptime() ? nullptr : value;
                    } else {
                        return value;
                    }
                }
                case ASTNodeKind::VarInitStmt: {
                    const auto init = linked->as_var_init_unsafe();
                    if(init->is_comptime()) {
                        return nullptr;
                    } else {
                        if(init->is_const()) {
                            const auto non_ct = getNonComptimeValueByKind(type, init->value);
                            if(non_ct) {
                                return value;
                            }
                            return nullptr;
                        } else {
                            return value;
                        }
                    }
                }
                default:
                    return value;
            }
        }
        case ValueKind::AccessChain:
            return getNonComptimeValueByKind(type, value->as_access_chain_unsafe()->values.back());
        case ValueKind::FunctionCall:{
            const auto call = value->as_func_call_unsafe();
            const auto linked = call->parent_val->linked_node();
            if(linked->kind() == ASTNodeKind::FunctionDecl) {
                const auto func = linked->as_function_unsafe();
                if(!func->is_comptime()) {
                    return value;
                }
                switch(func->returnType->canonical()->kind()) {
                    case BaseTypeKind::Runtime:
                    case BaseTypeKind::MaybeRuntime:
                        return value;
                    default:
                        return nullptr;
                }
            } else {
                return value;
            }
        }
        case ValueKind::Bool:
        case ValueKind::Double:
        case ValueKind::IntN:
        case ValueKind::String:
        case ValueKind::SizeOfValue:
        case ValueKind::AlignOfValue:
        case ValueKind::NullValue:
            return nullptr;
        case ValueKind::NegativeValue:
            return getNonComptimeValue(type, value->as_negative_value_unsafe()->getValue());
        case ValueKind::NotValue:
            return getNonComptimeValue(type, value->as_negative_value_unsafe()->getValue());
        case ValueKind::ArrayValue: {
            auto& values = value->as_array_value_unsafe()->values;
            if(values.empty()) {
                return nullptr;
            }
            const auto can_type = type->canonical();
            if(can_type->kind() == BaseTypeKind::Array) {
                const auto elem_type = can_type->as_array_type_unsafe()->known_child_type();
                if(isAnyRuntimeType(elem_type)) {
                    return nullptr;
                }
                for (const auto child_value: values) {
                    const auto child = getNonComptimeValue(elem_type, child_value);
                    if (child) {
                        return child;
                    }
                }
                return nullptr;
            } else {
                return value;
            }
        }
        case ValueKind::ExpressiveString: {
            return nullptr;
        }
        case ValueKind::CastedValue:
            return getNonComptimeValue(type, value->as_casted_value_unsafe()->value);
        default:
            return value;
    }
}

std::optional<bool> checkTypeIsComptime(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Literal:
            return true;
        case BaseTypeKind::Runtime:
            return false;
        case BaseTypeKind::Linked: {
            const auto l = type->as_linked_type_unsafe()->linked;
            if (l->kind() == ASTNodeKind::TypealiasStmt) {
                return checkTypeIsComptime(l->as_typealias_unsafe()->actual_type);
            } else {
                return std::nullopt;
            }
        }
        default:
            return std::nullopt;
    }
}

Value* getNonComptimeValue(BaseType* type, Value* value) {
    auto comptime_satisfies = checkTypeIsComptime(value->getType());
    if(comptime_satisfies.has_value()) {
        return comptime_satisfies.value() ? nullptr : value;
    }
    // is value a given that is not a reference ?
    return getNonComptimeValueByKind(type, value);
}

bool isAnyRuntimeType(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Runtime:
        case BaseTypeKind::MaybeRuntime:
            return true;
        case BaseTypeKind::Linked: {
            const auto l = type->as_linked_type_unsafe()->linked;
            if(l->kind() == ASTNodeKind::TypealiasStmt) {
                return isAnyRuntimeType(l->as_typealias_unsafe()->actual_type);
            } else {
                return false;
            }
        }
        default:
            return false;
    }
}

void verifyComptimeArgument(SymbolResolver& linker, BaseType* type, Value* value) {
    if(isAnyRuntimeType(type)) {
        return;
    }
    const auto nonComptimeValue = getNonComptimeValue(type, value);
    if(nonComptimeValue) {
        linker.error("comptime function expects argument that is known at compile time", nonComptimeValue);
    }
}

void verifyArgumentsAreComptime(SymbolResolver& linker, FunctionCall* call, FunctionType* func_type) {
    if(!func_type || !func_type->data.signature_resolved) return;
    unsigned i = 0;
    while(i < call->values.size()) {
        const auto param = func_type->func_param_for_arg_at(i);
        if (param) {
            const auto value = call->values[i];
            auto implicit_constructor = param->type->implicit_constructor_for(value);
            if (implicit_constructor) {
                verifyComptimeArgument(linker, implicit_constructor->params.front()->type, value);
            } else {
                verifyComptimeArgument(linker, param->type, value);
            }
        }
        i++;
    }
}

// can call a normal function
// can call a generic function (after instantiation, we can determine the type)
// can call a stored function pointer
// can call a capturing lambda function
// can call a struct which has a constructor
// can call a generic struct which has a constructor
// can call a variant member of a variant
// can call a variant member to instantiate a generic variant
bool link_call_without_parent(SymResLinkBody& visitor, FunctionCall* call, BaseType* expected_type, bool link_implicit_constructor) {

    auto& resolver = visitor.linker;

    GenericFuncDecl* gen_decl = nullptr;
    GenericVariantDecl* gen_var_decl = nullptr;

    const auto parent_val = call->parent_val;

    // relinking generic decl
    const auto parent_id = parent_val->get_last_id();
    if(parent_id) {

        const auto k = parent_id->linked->kind();

        if(k == ASTNodeKind::GenericFuncDecl) {

            gen_decl = parent_id->linked->as_gen_func_decl_unsafe();

            // we link with the master implementation currently, we still need to instantiate a new implementation
            parent_id->linked = gen_decl->master_impl;

        } else if(k == ASTNodeKind::VariantMember) {

            const auto mem = parent_id->linked->as_variant_member_unsafe();
            const auto mem_gen = mem->parent()->generic_parent;
            if(mem_gen != nullptr) {
                gen_var_decl = mem_gen->as_gen_variant_decl_unsafe();
                // we do not relink, because it's already linked with master implementation's variant member
                // parent_id->linked = gen_decl->master_impl;
            }

        }

    }

    const auto linked = parent_val->linked_node();
    // enum member being used as a no value
    const auto linked_kind = linked ? linked->kind() : ASTNodeKind::EnumMember;
    const auto func_decl = linked_kind == ASTNodeKind::FunctionDecl ? linked->as_function_unsafe() : nullptr;
    // TODO
//    if(func_decl) {
//        if(func_decl->is_unsafe() && resolver.safe_context) {
//            resolver.error("unsafe function with name should be called in an unsafe block", this);
//        }
//        const auto self_param = func_decl->get_self_param();
//        if(self_param) {
//            if(grandpa) {
//                if(self_param->type->is_mutable(BaseTypeKind::Reference)) {
//                    if(!first_value->check_is_mutable(resolver.current_func_type, resolver, false)) {
//                        resolver.error("call requires a mutable implicit self argument, however current self argument is not mutable", this);
//                    }
//                }
//            } else {
//                const auto arg_self = resolver.current_func_type->get_self_param();
//                if(!arg_self) {
//                    resolver.error("cannot call function without an implicit self arg which is not present", this);
//                } else if(self_param->type->is_mutable(BaseTypeKind::Reference) && !arg_self->type->is_mutable(BaseTypeKind::Reference)) {
//                    resolver.error("call requires a mutable implicit self argument, however current self argument is not mutable", this);
//                }
//            }
//        }
//    }

    call->relink_multi_func(resolver.allocator, &resolver);
    const auto gen_args_linked = link_call_gen_args(visitor, call);

    // link the values, based on which constructor is determined
    link_call_values(visitor, call);

    if(!gen_args_linked) {
        // link_constructor may try to register a generic instantiation
        // if generic args aren't linked, we don't want to do that
        return false;
    }

    // determine constructor being called
    // after this call, parent id should NOT be linked with a struct / generic struct / variant
    call->link_constructor(resolver.allocator, resolver.genericInstantiator);

    // check if variant member is not being called
    // then we must get a function type
    if(linked_kind != ASTNodeKind::VariantMember) {
        // if its not a variant, it should give us a function type to be valid
        // TODO: every function call type is being created using ast allocator
        const auto func_type = call->function_type();
        if(!func_type) {
            resolver.error(call) << "cannot call a non function type";
            call->setType(resolver.get_unresolved_decl()->known_type());
            return false;
        }
    }

    if(gen_decl || gen_var_decl) {
        // this will handle the generics
        goto instantiate_block;
    }

    // here to till ending_block, code runs for non generic things
    // figuring out the type for function call
    call->determine_type(*resolver.ast_allocator, resolver);

ending_block:
    if(link_implicit_constructor) {
        link_call_args_implicit_constructor(visitor, call);
    }

    {
        const auto func_type = resolver.current_func_type;
        if(func_type) {
            const auto func = func_type->as_function();
            if (!func || !func->is_comptime()) {
                // current function is not comptime
                // we only check this in runtime functions

                // now if a compile time function is being called
                // we verify all the arguments are known at compile time
                const auto final_linked = call->parent_val->linked_node();
                if (final_linked && final_linked->kind() == ASTNodeKind::FunctionDecl && final_linked->as_function_unsafe()->is_comptime()) {
                    verifyArgumentsAreComptime(resolver, call, final_linked->as_function_unsafe());
                }
            }
        }

    }

    return true;

instantiate_block:
    const auto func_type = resolver.current_func_type;
    const auto curr_func = func_type->as_function();
    // we don't want to put this call into it's own function's call subscribers it would lead to infinite cycle
    // we also don't want to instantiate this call, if the generic list is not completely specialized
    // TODO: two checks for specialization of parameters are running, one inside the instantiate_call
    // makeup mind about which one to keep
    if ((curr_func && curr_func->generic_parent != nullptr) || !are_all_specialized(call->generic_list)) {
        // since current function has a generic parent (it is generic), we do not want to instantiate this call here
        // this call will be instantiated by the instantiator, even if this calls itself (recursion), instantiator checks that
        // changing back to generic decl, since instantiator needs access to it
        parent_id->linked = gen_decl;
        call->setType(curr_func->returnType);
        return true;
    }
    if(gen_decl) {
        auto new_link = gen_decl->instantiate_call(resolver.genericInstantiator, call, expected_type);
        // instantiate call can return null, when the inferred types aren found to be not specialized
        if (!new_link) {
            parent_id->linked = gen_decl;
            call->setType(gen_decl->master_impl->returnType);
            return true;
        }

        // update linkage of parent identifier
        update_parent_val_linked(parent_val, new_link, new_link->known_type());
        // instantiated function's return type is the call's type
        call->setType(new_link->returnType);

        // if constructor function is being called, we must set the return type
        // to generic
        if(linked_kind == ASTNodeKind::FunctionDecl) {
            auto& allocator = *resolver.ast_allocator;
            if (func_decl->is_constructor_fn() && func_decl->parent()) {
                const auto struct_def = func_decl->parent()->as_struct_def();
                if (struct_def->generic_parent != nullptr) {
                    call->setType(new(allocator.allocate<GenericType>()) GenericType(new(allocator.allocate<LinkedType>()) LinkedType(struct_def)));
                }
            }
        }

    } else if(gen_var_decl) {
        auto new_link = gen_var_decl->instantiate_call(resolver.genericInstantiator, call, expected_type);
        if(!new_link) {
            // no re-linkage required, because it's already linked with master implementation
            return true;
        }
        const auto var_id = get_parent_from(parent_val);
        if(var_id) {
            // every variable is like this MyVariant.Member() <-- get parent from parent is MyVariant
            var_id->as_identifier()->linked = new_link;
        }
        const auto mem = parent_id->linked->as_variant_member_unsafe();
        const auto new_mem = new_link->direct_child(mem->name)->as_variant_member_unsafe();

        // update linkage of parent identifier
        parent_id->linked = new_mem;
        // set the type to be variant definition for current type
        auto& allocator = *resolver.ast_allocator;
        call->setType(
                new (allocator.allocate<GenericType>()) GenericType(new (allocator.allocate<LinkedType>()) LinkedType(new_mem->parent()), call->generic_list)
        );

    } else {
#ifdef DEBUG
        throw std::runtime_error("no condition satisfied in function call");
#endif
    }
    goto ending_block;
}

void SymResLinkBody::VisitFunctionCall(FunctionCall* call) {
    // load the expected type beforehand
    const auto exp_type = expected_type;
    const auto parent_val = call->parent_val;
    visit(parent_val, nullptr);
    link_call_without_parent(*this, call, exp_type, true);
}

void SymResLinkBody::VisitEmbeddedValue(EmbeddedValue* value) {
    auto traverser = linker.binder.findHook(value->name, CBIFunctionType::TraversalValue);
    if(traverser) {
        // traverse any nested values and set their types
        ((EmbeddedValueTraversalFunc) traverser)(value, this, embedded_traverse);
    } else {
        linker.error(value) << "couldn't find traversal method of embedded value with name '" << value->name << "' for symbol resolution";
    }
    auto found = linker.binder.findHook(value->name, CBIFunctionType::SymResValue);
    if(found) {
        ((EmbeddedValueSymbolResolveFunc) found)(&linker, value);
    } else {
        linker.error(value) << "couldn't find symbol resolve method for embedded value with name '" << value->name << "'";
    }
}

void SymResLinkBody::VisitComptimeValue(ComptimeValue* value) {
    visit(value->getValue(), expected_type);
    // type determined during symbol resolution needs to be set
    value->setType(value->getValue()->getType());
}

void SymResLinkBody::VisitIncDecValue(IncDecValue* value) {
    visit(value->getValue(), expected_type);
    // type determined at symbol resolution must be set
    value->setType(value->determine_type(linker));
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
        const auto number = evaluated->get_number();
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
            type->linked = linker.get_unresolved_decl();
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
        } else {
            // no need to error because we visited value, which prob cased an error if unresolved
            type->linked = linker.get_unresolved_decl();
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

void link_container(SymResLinkBody& visitor, VariablesContainer* container) {
    for(const auto var : container->variables()) {
        switch(var->kind()) {
            case ASTNodeKind::StructMember:
                visitor.visit(var->as_struct_member_unsafe()->type);
                break;
            case ASTNodeKind::UnnamedUnion:
                link_container(visitor, var->as_unnamed_union_unsafe());
                break;
            case ASTNodeKind::UnnamedStruct:
                link_container(visitor, var->as_unnamed_struct_unsafe());
                break;
            default:
#ifdef DEBUG
                throw std::runtime_error("unknown type of variable member");
#else
                continue;
#endif
        }
    }
}

void SymResLinkBody::VisitStructType(StructType* type) {
    type->take_variables_from_parsed_nodes(linker);
    link_container(*this, type);
    if(!type->name.empty()) {
        linker.declare(type->name, type);
    }
}

void SymResLinkBody::VisitUnionType(UnionType* type) {
    type->take_variables_from_parsed_nodes(linker);
    link_container(*this, type);
    if(!type->name.empty()) {
        linker.declare(type->name, type);
    }
}

void SymResLinkBody::VisitIfType(IfType* type) {
    visit(type->condition);
    visit(type->thenType);
    for(auto& pair : type->elseIfs) {
        visit(pair.first);
        visit(pair.second);
    }
    visit(type->elseType);
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
            case ASTNodeKind::VarInitStmt:
                if(linked->as_var_init_unsafe()->is_comptime()) {
                    linker.error("taking address of a comptime variable is not allowed", addrOfValue);
                }
                if(linked->as_var_init_unsafe()->is_const() && !linked->as_var_init_unsafe()->is_top_level()) {
                    linker.error("taking address of a constant is not allowed", addrOfValue);
                }
                break;
            default:
                break;
        }
    }

    // lets determine the type of this value
    addrOfValue->determine_type();

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
                const auto implicit = def->implicit_constructor_func(value);
                if(implicit) {
                    link_with_implicit_constructor(*this, implicit, value);
                } else if(!elemType->satisfies(value, false)) {
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
            known_elem_type = TypeLoc(value->getType(), known_elem_type.getLocation());
        }
        if(known_elem_type) {
            current_func_type.mark_moved_value(linker.allocator, value, known_elem_type, linker, elemType != nullptr);
            if(!known_elem_type->satisfies(value, false)) {
                linker.unsatisfied_type_err(value, known_elem_type);
            }
        }
        i++;
    }
}

void SymResLinkBody::VisitCastedValue(CastedValue* cValue) {
    const auto type = cValue->getType();
    const auto value = cValue->value;
    auto typeLoc = TypeLoc(type, cValue->type_location);
    visit(typeLoc);
    visit(value, type);
}

void SymResLinkBody::VisitDereferenceValue(DereferenceValue* value) {
    if(linker.safe_context) {
        linker.warn("de-referencing a pointer in safe context is prohibited", value);
    }
    visit(value->getValue());
    // determining the type for this dereference value
    auto& typeBuilder = linker.comptime_scope.typeBuilder;
    if(!value->determine_type(typeBuilder)) {
        linker.error("couldn't determine type for de-referencing", value);
    }
}

void SymResLinkBody::VisitExpression(Expression* value) {
    visit(value->firstValue);
    visit(value->secondValue);
    value->determine_type(linker.comptime_scope.typeBuilder, linker);
}

void SymResLinkBody::VisitIndexOperator(IndexOperator* indexOp) {

    // visiting stuff
    visit(indexOp->parent_val);
    visit(indexOp->idx);

    // determining the type for this index operator
    auto& typeBuilder = linker.comptime_scope.typeBuilder;
    indexOp->determine_type(typeBuilder, linker);

}

void SymResLinkBody::VisitIsValue(IsValue* isValue) {
    const auto value = isValue->value;
    auto& type = isValue->type;
    visit(value);
    visit(type);
}

void SymResLinkBody::VisitInValue(InValue* value) {
    visit(value->value);
    for(const auto val : value->values) {
        visit(val);
    }
}

BaseType* find_return_type(std::vector<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        if(node->kind() == ASTNodeKind::ReturnStmt) {
            auto returnStmt = node->as_return_unsafe();
            if(returnStmt->value) {
                return returnStmt->value->getType();
            } else {
                return nullptr;
            }
        } else {
            const auto loop_ast = node->as_loop_ast();
            if(loop_ast) {
                auto found = find_return_type(node->as_loop_ast()->body.nodes);
                if (found != nullptr) {
                    return found;
                }
            }
        }
    }
    return nullptr;
}

bool link_params_and_caps(LambdaFunction* fn, SymResLinkBody& visitor, bool link_param_types) {
    for(const auto cap : fn->captureList) {
        visitor.visit(cap);
    }
    bool result = true;
    for (auto& param : fn->params) {
        if(link_param_types) {
            link_param(visitor, param);
        }
        visitor.visit(param);
    }
    return result;
}

bool link_full(LambdaFunction* fn, SymResLinkBody& visitor, bool link_param_types) {
    auto& linker = visitor.linker;
    linker.scope_start();
    const auto result = link_params_and_caps(fn, visitor, link_param_types);
    link_seq(visitor, fn->scope);
    linker.scope_end();
    return result;
}

void copy_func_params_types(const std::vector<FunctionParam*>& from_params, std::vector<FunctionParam*>& to_params, SymResLinkBody& symRes, Value* debug_value) {
    auto& resolver = symRes.linker;
    if(to_params.size() > from_params.size()) {
        resolver.error(debug_value) << "Lambda function type expects " << std::to_string(from_params.size()) << " parameters however given " << std::to_string(to_params.size());
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
        auto result = link_full(lambVal, *this, true);

        // finding return type
        auto retType = find_return_type(scope.nodes);

        auto& typeBuilder = linker.comptime_scope.typeBuilder;
        returnType = {retType ? retType : typeBuilder.getVoidType(), lambVal->get_location()};

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

        if(link_full(lambVal, *this, false)) {
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
                identifier->setType(captured->linked->known_type());
                // we must move the identifiers in capture list
                prev_func_type->mark_moved_value(linker.allocator, identifier, captured->linked->known_type(), linker, false);
            }
        }

        lambVal->setIsCapturing(true);

    }

    linker.current_func_type = prev_func_type;

}

void SymResLinkBody::VisitNegativeValue(NegativeValue* negValue) {
    visit(negValue->getValue());
    // determine type for negative value
    negValue->determine_type(linker.comptime_scope.typeBuilder, linker);
}

void SymResLinkBody::VisitUnsafeValue(UnsafeValue* value) {
    const auto prev = linker.safe_context;
    linker.safe_context = false;
    visit(value->getValue(), expected_type);
    linker.safe_context = prev;
    value->setType(value->getValue()->getType());
}

void SymResLinkBody::VisitNewValue(NewValue* value) {
    visit(value->value);
    // type determined at symbol resolution must be set
    value->ptr_type.type = value->value->getType();
}

void SymResLinkBody::VisitTypeInsideValue(TypeInsideValue* value) {
    // TODO: we do not need to visit type inside value
    // because its created on demand by the generic instantiator
    visit(value->type, value->encoded_location());
}

void SymResLinkBody::VisitNewTypedValue(NewTypedValue* value) {
    visit(value->type);
}

void SymResLinkBody::VisitPlacementNewValue(PlacementNewValue* value) {
    visit(value->pointer);
    visit(value->value);
    // type of the value determined at symbol resolution must be set
    value->ptr_type.type = value->value->getType();
}

void SymResLinkBody::VisitNotValue(NotValue* value) {
    visit(value->getValue());
    // determine the type of not value
    value->determine_type(linker);
}

void SymResLinkBody::VisitPatternMatchExpr(PatternMatchExpr* expr) {
    // TODO: maybe pattern match expression should be a node
    // currently we emplace a void type
    // as expression is only used as a statement
    expr->setType(linker.comptime_scope.typeBuilder.getVoidType());
    // linking pattern match expression
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
    if(expr->destructure_by_name) {
        for (const auto nameId: param_names) {
            auto found = params.find(nameId->identifier);
            if (found == params.end()) {
                linker.error("couldn't find parameter in variant member", nameId);
            } else {
                nameId->member_param = found->second;
                // we declare this id, so anyone can link with it
                linker.declare(nameId->identifier, nameId);
            }
        }
    } else {
        auto begin = params.begin();
        auto end = params.end();
        for (const auto nameId: param_names) {
            if(begin == end) {
                linker.error("couldn't resolve the parameter by index", nameId);
                continue;
            } else {
                nameId->member_param = begin->second;
                // we declare this id, so anyone can link with it
                linker.declare(nameId->identifier, nameId);
            }
            begin++;
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

void SymResLinkBody::VisitIfValue(IfValue* value) {
    VisitIfStmt(&value->stmt);

    // we never reach this point if the statement is compile time
    // maybe type emplacement should also take place above
    auto last_val = value->stmt.get_value_node();
    if(last_val) {
        value->setType(last_val->getType());
    } else {
        linker.error("expected a single value node for the if value", value);
        return;
    }

}

void SymResLinkBody::VisitSwitchValue(SwitchValue* value) {
    VisitSwitchStmt(&value->stmt);

    // setting types for the given switch
    const auto node = value->stmt.get_value_node();
    if(node) {
        value->setType(node->getType());
    } else {
        linker.error("expected a single value node for the switch value", value);
        return;
    }


}

void SymResLinkBody::VisitLoopValue(LoopValue* value) {
    VisitLoopBlock(&value->stmt);
    // determine type of loop value
    const auto first = value->stmt.get_first_broken();
    if(first) {
        value->setType(first->getType());
    } else {
        value->setType(linker.comptime_scope.typeBuilder.getVoidType());
    }
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
    const auto refType = structValue->getRefType();
    if(refType) {
        visit(refType, structValue->encoded_location());
    } else {
        if(!exp_type) {
            linker.error("unnamed struct value cannot link without a type", structValue);
            structValue->setType(new (linker.ast_allocator->allocate<StructType>()) StructType("", nullptr, structValue->encoded_location()));
            return;
        }
        structValue->setType(exp_type);
    }
    if(!structValue->resolve_container(linker.genericInstantiator)) {
        return;
    }
    structValue->diagnose_missing_members_for_init(linker);
    if(!structValue->allows_direct_init()) {
        linker.error(structValue) << "struct with name '" << structValue->linked_extendable()->name_view() << "' has a constructor, use @direct_init to allow direct initialization";
    }
    auto refTypeKind = structValue->getRefType()->kind();
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
            current_func_type.mark_moved_value(linker.allocator, val.second.value, mem_type, linker);
            auto implicit = mem_type->implicit_constructor_for(val_ptr);
            if(implicit) {
                link_with_implicit_constructor(*this, implicit, val_ptr);
            } else if(!mem_type->satisfies(value, false)) {
                linker.unsatisfied_type_err(value, mem_type);
            }
        }
    }
}

void SymResLinkBody::VisitExpressiveString(ExpressiveString* value) {
    for(auto& val : value->values) {
        visit(val);
    }
}

void SymResLinkBody::VisitDynamicValue(DynamicValue* value) {
    visit(value->getType());
    visit(value->value);
    // TODO: must verify that an implementation exists
}