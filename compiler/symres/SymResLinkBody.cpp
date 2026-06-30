// Copyright (c) Chemical Language Foundation 2025.

#include "ast/statements/Assignment.h"
#include "ast/statements/UsingStmt.h"
#include "ast/statements/Export.h"
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
#include "ast/types/GenericType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
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

BaseType* SymResLinkBody::getErroredType() {
    return linker.comptime_scope.typeBuilder.getVoidType();
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
    const auto moved_ids_begin = visitor.moved_identifiers.size();
    const auto moved_chains_begin = visitor.moved_chains.size();
    visitor.VisitScope(&scope);
    if (nodes.back()->kind() == ASTNodeKind::ReturnStmt) {
        visitor.erase_moved_ids_after(moved_ids_begin);
        visitor.erase_moved_chains_after(moved_chains_begin);
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

    // where the moved ids / chains of if body begin
    const auto if_moved_ids_begin = visitor.moved_identifiers.size();
    const auto if_moved_chains_begin = visitor.moved_chains.size();

    // link the body
    link_seq(visitor, scope);

    // save all the moved identifiers / chains inside the if body to temporary location
    visitor.save_moved_ids_after(moved_ids, if_moved_ids_begin);
    visitor.save_moved_chains_after(moved_chains, if_moved_chains_begin);
}

void MembersContainer::declare_inherited_members(SymbolResolver& linker) {
    const auto container = this;
    for(const auto var : container->variables()) {
        linker.declare_or_shadow(var->name, var);
    }
    for (const auto func: container->evaluated_nodes()) {
        switch(func->kind()) {
            case ASTNodeKind::FunctionDecl:
                linker.declare_or_shadow(func->as_function_unsafe()->name_view(), func);
                break;
            case ASTNodeKind::GenericFuncDecl:
                linker.declare_or_shadow(func->as_gen_func_decl_unsafe()->name_view(), func);
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
        linker.declare_or_shadow(var->name, var);
    }
    for(const auto func : evaluated_nodes()) {
        switch(func->kind()) {
            case ASTNodeKind::FunctionDecl:
                linker.declare_or_shadow(func->as_function_unsafe()->name_view(), func);
                break;
            case ASTNodeKind::GenericFuncDecl:
                linker.declare_or_shadow(func->as_gen_func_decl_unsafe()->name_view(), func);
                break;
            default:
                break;
        }
    }
}

void SymResLinkBody::LinkMembersContainerNoScope(MembersContainer* container) {
    auto& inherited = container->inherited;
    for(auto& inherits : inherited) {
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
                    CHEM_THROW_RUNTIME("why does this variable has empty name");
            }
#endif
            continue;
        } else {
            linker.declare_or_shadow(var->name, var);
        }
    }
    SymbolResolverShadowDeclarer declarer(linker);
    // declare all the functions
    for(auto& func : container->evaluated_nodes()) {
        declare_node(declarer, func, AccessSpecifier::Private);
    }
    for (const auto func: container->evaluated_nodes()) {
        visit(func);
    }
}

void link_assignable(SymResLinkBody& symRes, Value* lhs, BaseType* expected_type) {
    switch(lhs->kind()) {
        case ValueKind::Identifier: {
            const auto prev = symRes.expected_type;
            symRes.expected_type = expected_type;
            symRes.VisitVariableIdentifier(lhs->as_identifier_unsafe(), false);
            symRes.expected_type = prev;
            break;
        }
        case ValueKind::AccessChain: {
            const auto prev = symRes.expected_type;
            symRes.expected_type = expected_type;
            symRes.VisitAccessChain(lhs->as_access_chain_unsafe(), true, true);
            symRes.expected_type = prev;
            break;
        }
        default:
            symRes.visit(lhs, expected_type);
    }
}

inline void link_val(SymResLinkBody &symRes, Value* value, BaseType* expected_type, bool assign) {
    if(assign && value->kind() == ValueKind::Identifier) {
        const auto id = value->as_identifier_unsafe();
        const auto prev = symRes.expected_type;
        symRes.expected_type = expected_type;
        symRes.VisitVariableIdentifier(id, false);
        symRes.expected_type = prev;
    } else {
        symRes.visit(value, expected_type);
    }
}

bool find_link_in_parent(VariableIdentifier* id, Value* parent, SymbolResolver& resolver) {
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
    const auto first_val = values[0];

    link_val(*this, first_val, values.size() == 1 ? exp_type : nullptr, assignment);

    // auto prepend self identifier, if not present and linked with struct member, anon union or anon struct
    if(first_val->kind() == ValueKind::Identifier) {
        const auto linked = first_val->as_identifier_unsafe()->linked;
        const auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::StructMember || linked_kind == ASTNodeKind::UnnamedUnion || linked_kind == ASTNodeKind::UnnamedStruct) {
            auto self_param = linker.current_func_type->get_self_param();
            if (!self_param) {
                //auto decl = linker.current_func_type->as_function();
                //if (!decl || !decl->is_constructor_fn() && !decl->is_comptime()) {
                linker.error(values[0]) << "unresolved identifier '" << values[0]->representation() << "', because function doesn't take a self argument";
                //}
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

    if(check_validity) {
        // check chain for validity, if it's moved or members have been moved
        check_chain(chain, assignment, linker);
    }
}

void SymResLinkBody::VisitVariableIdentifier(VariableIdentifier* identifier, bool check_access) {
    auto& value = identifier->value;
    if(in_lambda_scope) {
        auto sym = linker.find_bucket(value);
        if(sym == nullptr || sym->activeNode == nullptr) {
            // since we couldn't find a linked declaration, we will
            // link this identifier with unresolved declaration
            identifier->linked = linker.get_unresolved_decl();
            identifier->setType(identifier->linked->known_type());
            linker.error(identifier) << "unresolved variable identifier '" << value << "' not found";
            return;
        }
        if(sym->index < lambda_scope_start && !sym->activeNode->is_top_level()) {
            // since the symbol is outside lambda scope
            // we'll link this with unresolved declaration
            identifier->linked = linker.get_unresolved_decl();
            identifier->setType(identifier->linked->known_type());
            linker.error(identifier) << "symbol '" << value << "' is outside of lambda scope";
        } else {
            identifier->linked = sym->activeNode;
            identifier->setType(identifier->linked->known_type());
            if (check_access) {
                // check for validity if accessible or assignable (because moved)
                check_id(identifier, linker);
            }
            identifier->process_linked(&linker, linker.current_func_type);
            return;
        }
    }
    const auto linked = linker.find(value);
    if(linked) {
        identifier->linked = linked;
        identifier->setType(linked->known_type());
        if (check_access) {
            // check for validity if accessible or assignable (because moved)
            check_id(identifier, linker);
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

void SymResLinkBody::VisitAssignmentStmt(AssignStatement *assign) {
    const auto lhs = assign->lhs;
    const auto value = assign->value;

    link_assignable(*this, lhs, nullptr);

    const auto lhsType = lhs->getType();

    visit(value, lhsType);

    // check if operator is overloaded
    // direct assignment cannot be overloaded
    if(assign->assOp != Operation::Assignment) {
        const auto can_node = lhsType->get_linked_canonical_node(true, false);
        if(can_node) {
            const auto container = can_node->get_members_container();
            if(container) {
                const auto func = linker.implsIndex.get_ass_op_impl(linker.coreNodes, container, assign->assOp);
                if (func == nullptr) {
                    linker.error(assign) << "couldn't find overloaded operator implementation for assignment operator";
                    return;
                }
                if(func->params.size() != 2) {
                    linker.error("expected overload implementation to have exactly two parameters", assign);
                    return;
                }
                // check if rhs was moved and mark it
                mark_moved_value(linker.allocator, value, func->params[1]->known_type(), linker, true);
                return;
            }
        }
    }

    // we should report has assignment, even if it's a different operator
    // so the parameter can be allocated in a temporary variable for modification
    lhs->report_assignment_of_chain_id();

    // check if rhs was moved and mark it
    mark_moved_value(linker.allocator, value, lhs->getType(), linker, true);

    // unmove the lhs
    mark_un_moved_lhs_value(lhs, lhs->getType());

}

void UsingStmt::declare_symbols(SymbolResolver &linker) {
    auto linked = chain->get_chain_last_linked();
    if(!linked) {
        linker.error(this) << "couldn't find symbol '" << chain->representation() << "'";
        return;
    }
    if(is_namespace()) {
        auto ns = linked->as_namespace();
        if(ns) {
            for(auto& node_pair : ns->extended) {
                const auto node = node_pair.second;
                linker.declare_or_shadow(chem::string_view(node_pair.first.data(), node_pair.first.size()), node);
            }
        } else {
            linker.error("expected value to be a namespace, however it isn't", this);
        }
    } else {
        const auto& name_view = linked->get_node_identifier();
        if (!name_view.empty()) linker.declare_or_shadow(name_view, linked);
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

static bool isValidExportParent(ASTNode* parent) {
    switch(parent->kind()) {
        case ASTNodeKind::FileScope:
            return true;
        case ASTNodeKind::IfStmt:
            return isValidExportParent(parent->as_if_stmt_unsafe()->parent());
        default:
            return false;
    }
}

void SymResLinkBody::VisitExportStmt(ExportStmt* node) {
    if (node->parent() && !isValidExportParent(node->parent())) {
        linker.error("Export statement can only be used as a top level statement", node);
        return;
    }

}

void SymResLinkBody::VisitBreakStmt(BreakStatement* node) {
    if(node->value) {
        // TODO: we may need to pass expected type from
        //    where it is being assigned
        visit(node->value);
    }
    const auto loop_node = node->get_loop_node_above();
    if(loop_node) {
        node->target_loop_node = loop_node;
        loop_node->attrs.has_break = true;
    } else {
        linker.error("break statement requires a loop above that contains it", node);
    }
}

void SymResLinkBody::VisitDeleteStmt(DestructStmt* node) {
    const auto array_value = node->array_value;
    if(array_value) {
        visit(array_value);
    }
    visit(node->identifier);
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
        }
    }
}

VariantCase* create_variant_case(SymbolResolver& resolver, SwitchStatement* stmt, VariantDefinition* def, const chem::string_view id, SourceLocation loc) {
    auto& allocator = *resolver.ast_allocator;
    // explicit nullptr for ChildResolver, because we're just looking for direct variant members
    const auto child = def->child(nullptr, id);
    if(child) {
        if(child->kind() == ASTNodeKind::VariantMember) {
            return new (allocator.allocate<VariantCase>()) VariantCase(child->as_variant_member_unsafe(), stmt, resolver.comptime_scope.typeBuilder.getVoidType(), loc);
        } else {
            resolver.error(loc) << "couldn't find variant member with name '" << id << "'";
        }
    } else {
        resolver.error(loc) << "couldn't find the variant member with name '" << id << "'";
    }
    return nullptr;
}

VariantCase* create_variant_case(SymbolResolver& resolver, SwitchStatement* stmt, VariantDefinition* def, VariableIdentifier* id) {
    return create_variant_case(resolver, stmt, def, id->value, id->encoded_location());
}

void create_var_case_var(const chem::string_view& name, SymResLinkBody& linker, ASTAllocator& allocator, VariantCase* varCase, SwitchStatement* stmt, bool name_based, unsigned param_index, SourceLocation location) {
    const auto param = name_based ? varCase->member->values.find(name) : (varCase->member->values.begin() + param_index);
    if (param != varCase->member->values.end() && param_index < varCase->member->values.size()) {

        auto variable = new(allocator.allocate<VariantCaseVariable>()) VariantCaseVariable(name, param->second, stmt, location);
        varCase->identifier_list.emplace_back(variable);
        linker.visit(variable);

    } else {
        linker.linker.error("couldn't find variant member parameter with that name", location);
    }
}

void create_var_case_var(VariableIdentifier* id, SymResLinkBody& linker, ASTAllocator& allocator, VariantCase* varCase, SwitchStatement* stmt, bool name_based, unsigned param_index) {
    create_var_case_var(id->value, linker, allocator, varCase, stmt, name_based, param_index, id->encoded_location());
}

VariantCase* create_variant_case(SymResLinkBody& linker, SwitchStatement* stmt, VariantDefinition* def, FunctionCall* call) {
    auto& resolver = linker.linker;
    auto& astAlloc = *resolver.ast_allocator;
    if(call->parent_val->kind() == ValueKind::Identifier) {
        const auto first_id = call->parent_val->as_identifier_unsafe();
        const auto varCase = create_variant_case(resolver, stmt, def, first_id);
        if(varCase) {
            // put all values as variant case variables
            unsigned param_index = 0;
            for (const auto value: call->values) {
                if (value->kind() == ValueKind::Identifier) {
                    create_var_case_var(value->as_identifier_unsafe(), linker, astAlloc, varCase, stmt, false, param_index);
                } else if(value->kind() == ValueKind::AccessChain && value->as_access_chain_unsafe()->values.back()->kind() == ValueKind::Identifier) {
                    create_var_case_var(value->as_access_chain_unsafe()->values.back()->as_identifier_unsafe(), linker, astAlloc, varCase, stmt, false, param_index);
                } else {
                    resolver.error("expected value to be a identifier", value);
                }
                param_index++;
            }
            return varCase;
        }
        return nullptr;
    } else {
        resolver.error("expected first value in the function call to be identifier", call->parent_val);
    }
    return nullptr;
}

VariantCase* create_variant_case(SymResLinkBody& linker, SwitchStatement* stmt, VariantDefinition* def, StructValue* structVal) {
    auto& resolver = linker.linker;
    auto& astAlloc = *resolver.ast_allocator;
    const auto refType = structVal->getRefType();
    if (refType->kind() != BaseTypeKind::Linked || !refType->as_linked_type_unsafe()->is_named()) {
        linker.linker.error("unknown type in struct value for variant case", structVal);
        return nullptr;
    }
    const auto namedType = (NamedLinkedType*) refType;
    const auto varCase = create_variant_case(resolver, stmt, def, namedType->debug_link_name(), structVal->encoded_location());
    unsigned int index = 0;
    for (auto& val : structVal->values) {
        create_var_case_var(val.first, linker, astAlloc, varCase, stmt, true, index, structVal->encoded_location());
        index++;
    }
    return varCase;
}

void SymResLinkBody::VisitSwitchStmt(SwitchStatement *stmt) {

    const auto expression = stmt->expression;
    auto& scopes = stmt->scopes;
    auto& cases = stmt->cases;

    VariantDefinition* variant_def = nullptr;

    visit(expression);

    variant_def = stmt->getVarDefFromExpr();
    if(variant_def) {
        if (scopes.size() < variant_def->variables().size() && !stmt->has_default_case()) {
            linker.error("expected all cases of variant in switch statement when no default case is specified", (ASTNode*) stmt);
            return;
        }
        // currently only checking for the variant
        stmt->attrs.operating_on_closed_value = true;
    } else {
        // check it's a integer type
        // switching on float, double, reference and other structural types is not allowed
        if (!expression->getType()->isIntOrBoolLikeMarkedStorage()) {
            linker.error(expression) << "switch expression should have integer like type (integer or enum) but has " << expression->getType()->representation();
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
                        case ValueKind::StructValue: {
                            const auto varCase = create_variant_case(*this, stmt, variant_def, switch_case.first->as_struct_value_unsafe());
                            if (varCase) {
                                switch_case.first = varCase;
                            }
                            continue;
                        }
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
    restore_moved_ids(moved_ids);
    restore_moved_chains(moved_chains);

}

void SymResLinkBody::VisitTypealiasStmt(TypealiasStatement* node) {
    if(!node->is_top_level()) {
        linker.declare_node(node->name_view(), node, node->specifier(), false);
        visit(node->actual_type);
    }
    if(node->actual_type->kind() == BaseTypeKind::IfType) {
        const auto if_type = node->actual_type->as_if_type_unsafe();
        auto evaluated = if_type->evaluate(linker.comptime_scope);
        if(evaluated) {
            node->actual_type = evaluated;
        } else {
            linker.error("couldn't evaluate the if type", node->actual_type.encoded_location());
            node->actual_type = if_type->thenType;
        }
    }
}

void SymResLinkBody::VisitVarInitStmt(VarInitStatement* node) {
    if(node->is_top_level()) {
        return;
    }
    auto& type = node->type;
    const auto value = node->value;
    if (!type && !value) {
        return;
    }
    auto& attrs = node->attrs;
    if(type) {
        visit(type);
    }
    if(value) {
        visit(value, node->type_ptr_fast());
    }

    // special symbol declaration that checks for duplicate symbols
    linker.declare_local_var(node->id_view(), node, lambda_scope_start, in_lambda_scope);

    if (attrs.signature_resolved) {
        if(value) {
            mark_moved_value(linker.allocator, value, node->known_type(), linker, type != nullptr);
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
        }
    }
}

void SymResLinkBody::VisitComptimeBlock(ComptimeBlock* node) {
    link_seq(*this, node->body);
}

void verify_bool_ptr_condition(ASTDiagnoser& linker, BaseType* condType, SourceLocation loc) {
    const auto pure = condType->canonical();
    switch(pure->kind()) {
        case BaseTypeKind::IntN:
        case BaseTypeKind::Bool:
        case BaseTypeKind::NullPtr:
        case BaseTypeKind::Pointer:
        case BaseTypeKind::Function:
            return;
        default:
            linker.error("only integer / boolean / pointer types can be used as a condition", loc);

    }
}

void SymResLinkBody::VisitDoWhileLoopStmt(DoWhileLoop* node) {
    linker.scope_start();
    link_seq(*this, node->body);
    visit(node->condition);
    verify_bool_ptr_condition(linker, node->condition->getType(), node->condition->encoded_location());
    linker.scope_end();
}

void SymResLinkBody::VisitEnumMember(EnumMember* node) {
    if(node->init_value) {
        visit(node->init_value);
    }
    linker.declare_or_shadow(node->name, node);
}

void SymResLinkBody::VisitEnumDecl(EnumDeclaration* node) {
    auto& members = node->members;
    linker.scope_start();
    // since members is an unordered map, first we declare all enums
    // then we link their init values
    for(auto& mem : members) {
        linker.declare_or_shadow(mem.first, mem.second);
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

static void set_for_in_elem_type(ForInLoop* node, BaseType* elem_type) {
    if (node->is_reference()) {
        node->elem_type->as_reference_type_unsafe()->type = elem_type;
    } else {
        node->elem_type = elem_type;
    }
}

static BaseType* chunk_element_type(FunctionDeclaration* current_chunk_func) {
    BaseType* ret = current_chunk_func->returnType;
    if (ret->kind() == BaseTypeKind::Generic) {
        const auto gen = ret->as_generic_type_unsafe();
        if (!gen->types.empty()) {
            return gen->types.front();
        }
    }
    return nullptr;
}

void SymResLinkBody::VisitForInLoopStmt(ForInLoop* node) {
    linker.scope_start();
    visit(node->expr);

    const auto exprTy = node->expr->getType();
    const auto type = exprTy->canonical();
    if (type->kind() == BaseTypeKind::Array) {
        const auto arrType = type->as_array_type_unsafe();
        const auto arrSize = arrType->get_array_size();
        if (arrSize == 0) {
            linker.error("array size is not known at compile time so it cannot be iterated", node->expr);
            linker.hint(node->expr->encoded_location()) << "use a std::span to iterate over an array";
            return;
        }
        if (node->is_reference()) {
            set_for_in_elem_type(node, arrType->elem_type);
        } else {
            set_for_in_elem_type(node, arrType->elem_type);
        }
        node->iteration_kind = ForInLoopIterationKind::Array;
    } else {
        const auto linked = type->get_linked_node(true, false);
        if (!linked) {
            linker.error("couldn't get container from expression", node->expr);
            return;
        }
        const auto container = linked->get_members_container();
        if (!container) {
            linker.error("couldn't get container from expression", node->expr);
            return;
        }
        const auto iterDataFunc = linker.implsIndex.get_linear_data_impl(linker.coreNodes, container);
        if (iterDataFunc) {
            const auto iterSizeFunc = linker.implsIndex.get_linear_size_impl(linker.coreNodes, container);
            if (!iterSizeFunc) {
                linker.error("couldn't get 'core::iterable::Linear::size' implementation", node->expr);
                return;
            }
            if (iterDataFunc->returnType->kind() != BaseTypeKind::Pointer) {
                linker.error("expected 'core::iterable::Linear::data' return type to be a pointer", node->expr);
                return;
            }
            const auto ty = iterDataFunc->returnType->as_pointer_type_unsafe()->type;
            set_for_in_elem_type(node, ty);
            node->iteration_kind = ForInLoopIterationKind::Linear;
        } else {
            const auto chunkCurrentFunc = linker.implsIndex.get_chunked_current_chunk_impl(linker.coreNodes, container);
            if (chunkCurrentFunc) {
                const auto elem = chunk_element_type(chunkCurrentFunc);
                if (!elem) {
                    linker.error("expected 'core::iterable::Chunked::current_chunk' to return core::iterable::Chunk<T>", node->expr);
                    return;
                }
                set_for_in_elem_type(node, elem);
                node->iteration_kind = ForInLoopIterationKind::Chunked;
            } else {
                const auto iterableCurrentFunc = linker.implsIndex.get_iterable_current_impl(linker.coreNodes, container);
                if (!iterableCurrentFunc) {
                    linker.error("expected container to implement 'core::iterable::Linear', 'core::iterable::Chunked', or 'core::iterable::Iterable'", node->expr);
                    return;
                }
                if (node->is_reversed()) {
                    const auto rbeginFunc = linker.implsIndex.get_reversible_iterable_rbegin_impl(linker.coreNodes, container);
                    const auto previousFunc = linker.implsIndex.get_reversible_iterable_previous_impl(linker.coreNodes, container);
                    const auto countFunc = linker.implsIndex.get_reversible_iterable_count_impl(linker.coreNodes, container);
                    if (!rbeginFunc || !previousFunc || !countFunc) {
                        linker.error("reversed iteration requires 'core::iterable::ReversibleIterable' implementation", node->expr);
                        return;
                    }
                }
                if (iterableCurrentFunc->returnType->kind() != BaseTypeKind::Reference) {
                    linker.error("expected 'core::iterable::Iterable::current' return type to be a reference", node->expr);
                    return;
                }
                const auto ty = iterableCurrentFunc->returnType->as_reference_type_unsafe()->type;
                set_for_in_elem_type(node, ty);
                node->iteration_kind = ForInLoopIterationKind::Iterable;
            }
        }
    }

    // we declare the same id
    linker.declare(node->id, node);
    if (node->index_init != nullptr) {
        linker.declare(node->index_init->id_view(), node->index_init);
    }

    link_seq(*this, node->body);
    linker.scope_end();
}

void SymResLinkBody::VisitFunctionParam(FunctionParam* node) {
    linker.declare_or_shadow(node->name, node);
}

void SymResLinkBody::VisitGenericTypeParam(GenericTypeParameter* node) {
    for(auto& t : node->traits) {
        visit(t);
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
    if(name == "self") { // name and other means pointers to parent node
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

const auto missing_return_err_msg = "missing return for function that has a non void return type";

void verify_has_return(SymbolResolver& linker, Scope& scope, SourceLocation location) {
    if(scope.nodes.empty()) {
        linker.error(missing_return_err_msg, location);
        return;
    }
    // go from the back to verify nodes
    const auto last = scope.nodes.back();
    switch(last->kind()) {
        case ASTNodeKind::ReturnStmt:
        case ASTNodeKind::UnreachableStmt:
            return;
        case ASTNodeKind::IfStmt: {
            const auto stmt = last->as_if_stmt_unsafe();
            if (!stmt->elseBody.has_value()) {
                linker.error("missing return in else body for function that has non void return type", stmt->encoded_location());
                return;
            }
            verify_has_return(linker, stmt->ifBody, stmt->ifBody.encoded_location());
            for(auto& elseIf : stmt->elseIfs) {
                verify_has_return(linker, elseIf.second, elseIf.second.encoded_location());
            }
            verify_has_return(linker, stmt->elseBody.value(), stmt->elseBody.value().encoded_location());
            return;
        }
        case ASTNodeKind::SwitchStmt:{
            const auto stmt = last->as_switch_stmt_unsafe();
            if(stmt->defScopeInd == -1 && !stmt->attrs.operating_on_closed_value) {
                linker.error("missing default case where switch is the last statement of the function", stmt->encoded_location());
            }
            for(auto& child_scope : stmt->scopes) {
                verify_has_return(linker, child_scope, child_scope.encoded_location());
            }
            return;
        }
        case ASTNodeKind::LoopBlock:{
            const auto stmt = last->as_loop_block_unsafe();
            if(!stmt->attrs.has_break) {
                return;
            }
            break;
        }
        case ASTNodeKind::ProvideStmt: {
            const auto stmt = (ProvideStmt*) last;
            verify_has_return(linker, stmt->body, stmt->body.encoded_location());
            return;
        }
        case ASTNodeKind::UnsafeBlock: {
            const auto blk = last->as_unsafe_block_unsafe();
            verify_has_return(linker, blk->scope, blk->scope.encoded_location());
            return;
        }
        case ASTNodeKind::ComptimeBlock: {
            const auto blk = (ComptimeBlock*) last;
            verify_has_return(linker, blk->body, blk->body.encoded_location());
            return;
        }
        default:
            break;
    }
    linker.error(missing_return_err_msg, location);
}

void SymResLinkBody::VisitFunctionDecl(FunctionDeclaration* node) {
    if(node->body.has_value()) {
        // if has body declare params
        linker.scope_start();

        // save the function type
        auto prev_func_type = linker.current_func_type;
        linker.current_func_type = node;

        // save moved ids and clear so we
        auto prev_moved_ids = moved_identifiers;
        auto prev_moved_chains = moved_chains;
        moved_identifiers.clear();
        moved_chains.clear();

        // linking
        for (const auto param : node->params) {
            visit(param);
        }
        if(node->FunctionType::data.signature_resolved) {
            if(node->is_comptime()) {
                linker.comptime_context = true;
            }
            link_seq(*this, node->body.value());
            if(node->returnType->canonical()->kind() != BaseTypeKind::Void) {
                verify_has_return(linker, node->body.value(), node->encoded_location());
            }
            linker.comptime_context = false;
        }
        linker.scope_end();

        // restore previous moved ids and chains
        moved_identifiers.clear();
        moved_chains.clear();
        moved_identifiers.insert(moved_identifiers.end(), prev_moved_ids.begin(), prev_moved_ids.end());
        moved_chains.insert(moved_chains.end(), prev_moved_chains.begin(), prev_moved_chains.end());

        // retore the function type
        linker.current_func_type = prev_func_type;
    }

}

void SymResLinkBody::VisitInterfaceDecl(InterfaceDefinition* node) {
    LinkMembersContainer(node);
}

void SymResLinkBody::VisitStructDecl(StructDefinition* node) {
    LinkMembersContainer(node);
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
    linker.declare_or_shadow(node->name, node);
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

void link_conditions_no_patt_match_expr(IfStatement* stmt, SymResLinkBody &symRes) {
    if(stmt->condition->kind() != ValueKind::PatternMatchExpr) {
        symRes.visit(stmt->condition);
        verify_bool_ptr_condition(symRes.linker, stmt->condition->getType(), stmt->condition->encoded_location());
    }
    for (auto& cond: stmt->elseIfs) {
        if(cond.first->kind() != ValueKind::PatternMatchExpr) {
            symRes.visit(cond.first);
            verify_bool_ptr_condition(symRes.linker, cond.first->getType(), cond.first->encoded_location());
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

    if (node->is_top_level()) {
        // must not re-evaluate comptime top level if statement
        // it is evaluated in declare top level
        if (node->computed_scope.has_value()) {
            const auto scope = node->computed_scope.value();
            if (scope) {
                visit(scope);
            }
        }
        return;
    }

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

    restore_moved_ids(moved_ids);
    restore_moved_chains(moved_chains);

}

void SymResLinkBody::VisitImplDecl(ImplDefinition* node) {
    const auto linked_node = node->interface_type->get_direct_linked_canonical_node();
    const auto linked = linked_node ? linked_node->as_interface_def() : nullptr;
    linker.scope_start();
    const auto struct_linked = node->struct_type ? node->struct_type->get_direct_linked_struct() : nullptr;
    if(linked) {
        for (const auto func: linked->evaluated_nodes()) {
            switch(func->kind()) {
                case ASTNodeKind::FunctionDecl:
                    linker.declare_or_shadow(func->as_function_unsafe()->name_view(), func);
                    break;
                case ASTNodeKind::GenericFuncDecl:
                    linker.declare_or_shadow(func->as_gen_func_decl_unsafe()->name_view(), func);
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
        if (linked) struct_linked->adopt(linked);
    }
    LinkMembersContainerNoScope(node);
    linker.scope_end();
}

void SymResLinkBody::VisitNamespaceDecl(Namespace* node) {
    linker.scope_start();
    if(node->root) {
        node->root->declare_extended_in_linker(linker);
    } else {
        node->declare_extended_in_linker(linker);
        SymbolResolverShadowDeclarer declarer(linker);
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

void SymResLinkBody::VisitBlockScope(BlockScope* node) {
    linker.scope_start();
    for (const auto child: node->nodes) {
        visit(child);
    }
    linker.scope_end();
}

void SymResLinkBody::VisitLoopBlock(LoopBlock* node) {
    link_seq(*this, node->body);

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
    // const auto member = node->member_param->parent();
    // auto child = member->values.find(node->name);
    // if(child == member->values.end()) {
    //     linker.error(node) << "variant case member variable not found in switch statement, name '" << node->name << "' not found";
    //     return;
    // }
    // node->member_param = child->second;
    linker.declare_or_shadow(node->name, node);
}

void SymResLinkBody::VisitWhileLoopStmt(WhileLoop* node) {
    linker.scope_start();
    visit(node->condition);
    verify_bool_ptr_condition(linker, node->condition->getType(), node->condition->encoded_location());
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

void SymResLinkBody::VisitEmbeddedNode(EmbeddedNode* node) {
    auto found = linker.binder.findHook(node->name, CBIFunctionType::SymResNode);
    if(found) {
        ((EmbeddedNodeSymbolResolveFunc) found)(this, node);
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

    const auto parent = parent_val->get_chain_last_linked();
    if(parent) {
        const auto variant_mem = parent->as_variant_member();
        if (variant_mem) {

            unsigned i = 0;
            const auto values_size = values.size();
            const auto total_params = variant_mem->values.size();
            while (i < values_size) {
                auto& value_ptr = values[i];
                auto& value = *value_ptr;
                if(i < total_params) {
                    const auto param = (variant_mem->values.begin() + i)->second;
                    const auto expected_type = param->type;
                    visitor.visit(&value, expected_type);
                    visitor.mark_moved_value(linker.allocator, &value, expected_type, linker);
                } else {
                    linker.error(value_ptr) << "too many arguments given, expected " << std::to_string(total_params) << " given " << std::to_string(values_size);
                }
                i++;
            }

            // checking arguments exist for all variant call parameters
            while (i < total_params) {
                auto param = (variant_mem->values.begin() + i)->second;
                if (param && param->def_value == nullptr) {
                    linker.error(call) << "variant call parameter '" << param->name << "' doesn't have a default value and no argument exists for it";
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

    // checking arguments exist for all function call values
    if(func_type) {

        const auto func_param_size = func_type->expectedArgsSize();

        unsigned i = 0;
        const auto values_size = values.size();
        while (i < values_size) {
            auto& value = *values[i];
            const auto param = func_type->func_param_for_arg_at(i);
            if(param) {
                const auto expected_type = param->type;
                visitor.visit(&value, expected_type);
                visitor.mark_moved_value(linker.allocator, &value, expected_type, linker);
            } else {
                linker.error(&value) << "too many arguments given, expected " << std::to_string(func_param_size) << " given " << std::to_string(values_size);
            }
            i++;
        }

        while (i < func_param_size) {
            auto param = func_type->func_param_for_arg_at(i);
            if (param) {
                if (param->defValue == nullptr && !func_type->isInVarArgs(i)) {
                    linker.error(call) << "function parameter '" << param->name << "' doesn't have a default value and no argument exists for it";
                }
            }
            i++;
        }

    } else {

        unsigned i = 0;
        const auto values_size = values.size();
        while (i < values_size) {
            auto& value_ptr = values[i];
            auto& value = *value_ptr;
            // expected_type -> nullptr (because user is probably calling constructor, and we can only know which constructor to call, after arguments are linked and their type is known)
            visitor.visit(&value, nullptr);
            visitor.mark_moved_value(linker.allocator, &value, nullptr, linker);
            i++;
        }

    }

}

void link_call_args_implicit_constructor(SymResLinkBody& visitor, FunctionCall* call){
    auto& linker = visitor.linker;

    const auto parent = call->parent_val->get_chain_last_linked();
    if(parent) {
        const auto variant_mem = parent->as_variant_member();
        if (variant_mem) {

            unsigned i = 0;
            const auto values_size = call->values.size();
            const auto total_params = variant_mem->values.size();
            while (i < values_size) {
                const auto value_ptr = call->values[i];
                if(i < total_params) {
                    const auto param = (variant_mem->values.begin() + i)->second;
                    auto implicit_constructor = param->type->implicit_constructor_for(value_ptr);
                    if (implicit_constructor) {
                        link_with_implicit_constructor(visitor, implicit_constructor, value_ptr);
                    }
                } else {
                    linker.error(value_ptr) << "too many arguments given, expected " << std::to_string(total_params) << " given " << std::to_string(values_size);
                }
                i++;
            }

            return;
        }
    }

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

void update_parent_val_linked(Value* value, ASTNode* node, BaseType* type) {
    switch(value->kind()) {
        case ValueKind::Identifier:
            value->as_identifier_unsafe()->linked = node;
            value->as_identifier_unsafe()->setType(type);
            return;
        case ValueKind::AccessChain: {
            const auto last = value->as_access_chain_unsafe()->values.back();
#ifdef DEBUG
            if(last->kind() != ValueKind::Identifier) {
                CHEM_THROW_RUNTIME("this should always be an id");
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

void materialize_lambda(
        LambdaFunction* func,
        ASTDiagnoser& diagnoser,
        FunctionType* func_type
) {
    func->returnType = func_type->returnType;
    const auto& from_params = func_type->params;
    auto& to_params = func->params;
    auto total = from_params.size();
    auto start = 0;
    while(start < total) {
        const auto from_param = from_params[start];
        if(start >= to_params.size()) {
            return;
        }
        to_params[start]->type = from_param->type;
        start++;
    }
}

// probably won't use allocator (see materialize_lambda_parameters)
void materialize_argument_type(ASTAllocator& allocator, ASTDiagnoser& diagnoser, BaseType* expected_type, Value* arg) {
    switch(arg->kind()) {
        case ValueKind::LambdaFunc: {
            const auto func = arg->as_lambda_func_unsafe();
            const auto func_type = expected_type->get_canonical_function_type();
            if(func_type) {
                materialize_lambda(arg->as_lambda_func_unsafe(), diagnoser, func_type);
            } else {
                diagnoser.error("function doesn't expect a lambda function argument for this parameter", arg);
            }
        }
        default:
            return;
    }
}

void FunctionCall::report_concrete_types(ASTAllocator& allocator, ASTDiagnoser& diagnoser) {

    const auto call = this;
    const auto parent_val = call->parent_val;
    auto& values = call->values;

    const auto parent = parent_val->get_chain_last_linked();
    if(parent) {
        const auto variant_mem = parent->as_variant_member();
        if (variant_mem) {
            unsigned i = 0;
            const auto values_size = values.size();
            const auto total_params = variant_mem->values.size();
            while (i < values_size) {
                const auto param = i < total_params ? (variant_mem->values.begin() + i)->second : nullptr;
                const auto expected_type = param ? param->type : nullptr;
                materialize_argument_type(allocator, diagnoser, expected_type, values[i]);
                i++;
            }

            return;
        }
    }

    auto func_type = call->function_type();
    if (func_type && !func_type->data.signature_resolved) {
        return;
    }

    unsigned i = 0;
    const auto values_size = values.size();
    while (i < values_size) {
        const auto param = func_type ? func_type->func_param_for_arg_at(i) : nullptr;
        const auto expected_type = param ? param->type : nullptr;
        materialize_argument_type(allocator, diagnoser, expected_type, values[i]);
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

    const auto linked = parent_val->get_chain_last_linked();
    // enum member being used as a no value
    const auto linked_kind = linked ? linked->kind() : ASTNodeKind::EnumMember;
    const auto func_decl = linked_kind == ASTNodeKind::FunctionDecl ? linked->as_function_unsafe() : nullptr;

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
    call->link_constructor(resolver.allocator, resolver.genericInstantiator, !resolver.generic_context);

    // check if variant member is not being called
    // then we must get a function type
    if(linked_kind != ASTNodeKind::VariantMember) {
        // if its not a variant, it should give us a function type to be valid
        const auto func_type = call->get_function_type_during_linking();
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
    return true;

instantiate_block:
    const auto func_type = resolver.current_func_type;
    const auto curr_func = func_type->as_function();
    if ((curr_func && curr_func->generic_parent != nullptr) || resolver.generic_context) {
        // since current function has a generic parent (it is generic), we do not want to instantiate this call here
        // this call will be instantiated by the generic instantiator, even if this calls itself (recursion), instantiator checks that
        // changing back to generic decl, since instantiator needs access to it
        if(gen_decl) {
            parent_id->linked = gen_decl;
            call->setType(gen_decl->master_impl->returnType);
        } else {
            if(gen_var_decl) {
                call->setType(gen_var_decl->known_type());
            }
        }
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

        // report concrete types, for example lambda functions
        // so they know the expected types
        call->report_concrete_types(*resolver.ast_allocator, resolver);

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
        CHEM_THROW_RUNTIME("no condition satisfied in function call");
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
    // we miss setting the type of the function call (even though we shouldn't)
    // therefore this checks and fixes that
    if (call->getType() == nullptr) {
        call->setType(linker.get_unresolved_decl()->known_type());
    }
}

void SymResLinkBody::VisitEmbeddedValue(EmbeddedValue* value) {
    auto found = linker.binder.findHook(value->name, CBIFunctionType::SymResValue);
    if(found) {
        ((EmbeddedValueSymbolResolveFunc) found)(this, value);
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
    const auto val = value->getValue();
    visit(val, expected_type);
    // report assignment, if this is a linked parameter / var init
    val->report_assignment_of_chain_id();
    // type determined at symbol resolution must be set
    value->setType(value->determine_type(linker, linker.coreNodes, linker.implsIndex));
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
        if(param->type) {
            symRes.visit(param->type);
        }
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
    if(linker.generic_context) {
        gen_type->instantiate_inline(linker.genericInstantiator, type_location);
    } else {
        gen_type->instantiate(linker.genericInstantiator, type_location);
    }
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
        const auto linked = value->get_chain_last_linked();
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

void link_container(SymResLinkBody& visitor, VariablesContainerBase* container) {
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
                CHEM_THROW_RUNTIME("unknown type of variable member");
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
    value->report_addr_taken_of_chain_id();
    // lets determine the type of this value
    addrOfValue->determine_type();

}

void SymResLinkBody::VisitReferenceOfValue(ReferenceOfValue* refValue) {
    const auto value = refValue->value;
    visit(value);
    value->report_addr_taken_of_chain_id();
    // lets determine the type of this value
    refValue->determine_type();
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
        if(elemType == nullptr) {
            elemType = arr_type->elem_type.copy(*linker.ast_allocator);
        }
        const auto valType = arrValue->getType();
        if(valType->has_array_size() && arr_type->has_no_array_size()) {
            arr_type->set_array_size(valType->get_array_size());
        } else if(valType->has_no_array_size() && arr_type->has_array_size()) {
            valType->set_array_size(arr_type->get_array_size());
        }
    }
    if(elemType) {
        const auto def = elemType->get_direct_linked_struct();
        if(def) {
            unsigned i = 0;
            while (i < values.size()) {
                auto& val_ptr = values[i];
                const auto value = val_ptr;
                visit(value, elemType);
                const auto implicit = def->implicit_constructor_func(value);
                if(implicit) {
                    link_with_implicit_constructor(*this, implicit, value);
                }
                i++;
            }
            return;
        }
    }
    auto& known_elem_type = elemType;
    unsigned i = 0;
    for(auto& value : values) {
        visit(value);
        if(i == 0 && !known_elem_type) {
            known_elem_type = TypeLoc(value->getType(), known_elem_type.getLocation());
        }
        if(known_elem_type) {
            mark_moved_value(linker.allocator, value, known_elem_type, linker, elemType != nullptr);
        }
        i++;
    }
    if(known_elem_type == nullptr) {
        known_elem_type = TypeLoc(getErroredType(), arrValue->encoded_location());
        linker.error("couldn't determine element type for array", arrValue);
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
    visit(value->firstValue, nullptr);
    visit(value->secondValue, nullptr);
    value->determine_type(
        linker.comptime_scope.typeBuilder,
        linker.coreNodes,
        linker.implsIndex,
        linker,
        linker.comptime_scope.target_data
    );
}

void SymResLinkBody::VisitIndexOperator(IndexOperator* indexOp) {

    // visiting stuff
    visit(indexOp->parent_val);
    visit(indexOp->idx);

    // determining the type for this index operator
    auto& typeBuilder = linker.comptime_scope.typeBuilder;
    indexOp->determine_type(typeBuilder, linker.coreNodes, linker.implsIndex, linker);

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

bool link_params_and_caps(LambdaFunction* fn, SymResLinkBody& visitor) {
    for(const auto cap : fn->captureList) {
        visitor.visit(cap);
    }
    bool result = true;
    for (auto& param : fn->params) {
        link_param(visitor, param);
        visitor.visit(param);
    }
    return result;
}

void link_lambda_body(unsigned long scope_index, LambdaFunction* fn, SymResLinkBody& visitor) {
    auto scope = visitor.linker.get_scope_at_index(scope_index);
    auto scope_start_index = scope->start;

    auto prev_in_lamb_scope = visitor.in_lambda_scope;
    visitor.in_lambda_scope = true;

    auto prev_scope_start = visitor.lambda_scope_start;
    visitor.lambda_scope_start = scope_start_index;
    link_seq(visitor, fn->scope);

    visitor.in_lambda_scope = prev_in_lamb_scope;
    visitor.lambda_scope_start = prev_scope_start;
}

bool link_full(LambdaFunction* fn, SymResLinkBody& visitor) {
    auto& linker = visitor.linker;
    auto scope_index = linker.scope_start_index();
    const auto result = link_params_and_caps(fn, visitor);
    link_lambda_body(scope_index, fn, visitor);
    linker.scope_end();
    return result;
}

void copy_func_params_types(
        const std::vector<FunctionParam*>& from_params,
        std::vector<FunctionParam*>& to_params,
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser,
        Value* debug_value
) {
    if(to_params.size() > from_params.size()) {
        diagnoser.error(debug_value) << "Lambda function type expects " << std::to_string(from_params.size()) << " parameters however given " << std::to_string(to_params.size());
        return;
    }
    auto total = from_params.size();
    auto start = 0;
    while(start < total) {
        const auto from_param = from_params[start];
        bool verify_type = true;
        if(start >= to_params.size()) {
            // copying since user didn't include the parameter
            to_params.emplace_back(from_param->copy(allocator));
            verify_type = false;
        }
        const auto to_param = to_params[start];
        if(!to_param || !to_param->type || to_param->is_implicit() || from_param->is_implicit()) {
            const auto copied = to_param;
            if(!to_param->type) {
                to_param->type = from_param->type;
            }
            if(!to_param->defValue && from_param->defValue) {
                to_param->defValue = from_param->defValue;
            }
            to_param->attrs = from_param->attrs;
            // TODO: assigning a name to a parameter may not have asked for
            //  this would cause a symbol in the scope that user doesn't know about
            //  we should keep this name empty or make an attribute for unused parameter
            copied->name = to_param->name;
        } else if(verify_type) {
            // user gave the type
            assert(to_param->type != nullptr);
            if(!from_param->type->is_same(to_param->type)) {
                diagnoser.error("Lambda function parameter type mismatch", to_param);
            }
        }
        start++;
    }
}

bool link_lambda(
        LambdaFunction* func,
        ASTAllocator& allocator,
        ASTDiagnoser& diagnoser,
        FunctionType* func_type
) {
    auto& params = func->params;
    auto& returnType = func->returnType;
    copy_func_params_types(func_type->params, params, allocator, diagnoser, func);
    if(!returnType) {
        returnType = func_type->returnType;
    } else if(!returnType->is_same(func_type->returnType)) {
        diagnoser.error((Value*) func) << "Lambda function type expected return type to be " << func_type->returnType->representation() << " but got lambda with return type " << returnType->representation();
    }
    func->setIsCapturing(func_type->isCapturing());
    return true;
}

FunctionType* get_func_type_from_exp_type(BaseType* type) {
    switch(type->kind()) {
        case BaseTypeKind::Function:
            return (FunctionType*) type;
        case BaseTypeKind::CapturingFunction:
            return type->as_capturing_func_type_unsafe()->func_type->as_function_type();
        case BaseTypeKind::Reference:
            return get_func_type_from_exp_type(type->as_reference_type_unsafe()->type);
        default:
            return nullptr;
    }
}

void SymResLinkBody::VisitLambdaFunction(LambdaFunction* lambVal) {

    auto& scope = lambVal->scope;
    auto& returnType = lambVal->returnType;
    auto& data = lambVal->data;
    auto& params = lambVal->params;
    auto& captureList = lambVal->captureList;

    // set the current function type
    auto prev_func_type = linker.current_func_type;
    linker.current_func_type = lambVal;

    const auto canonical_exp = expected_type ? expected_type->canonical() : nullptr;
    auto func_type = canonical_exp ? get_func_type_from_exp_type(canonical_exp) : nullptr;

    // before we start linking lambda, we must save current moved identifiers and chains
    auto prev_moved_ids = moved_identifiers;
    auto prev_moved_chains = moved_chains;

    // clear the current
    moved_identifiers.clear();
    moved_chains.clear();

    if(!func_type) {

        // linking params and their types
        auto result = link_full(lambVal, *this);

        if (lambVal->returnType == nullptr) {
#ifdef DEBUG
            linker.info("deducing lambda function type by visiting body", (Value*) lambVal);
#endif

            // finding return type
            auto retType = find_return_type(scope.nodes);

            auto& typeBuilder = linker.comptime_scope.typeBuilder;
            returnType = {retType ? retType : typeBuilder.getVoidType(), lambVal->get_location()};
        }

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

        // start the scope
        auto scope_index = linker.scope_start_index();

        // link parameter and captured variables (only the ones user gave)
        const auto result = link_params_and_caps(lambVal, *this);
        if(result) {
            data.signature_resolved = true;
        }

        // will also create parameters that don't exist, assign types to parameters not given
        // assigned types won't be visited (assumed linked)
        link_lambda(lambVal, *linker.ast_allocator, linker, func_type);

        // link the body
        link_lambda_body(scope_index, lambVal, *this);

        // end the scope, which drops both (parameters and any symbols in lambda body)
        linker.scope_end();

    }

    if(captureList.empty()) {

        if(canonical_exp) {
            const auto capFunc = canonical_exp->get_cap_func_type();
            if(capFunc) {
                lambVal->setIsCapturing(true);
            }
        }

    } else {

        if(canonical_exp) {
            const auto capFunc = canonical_exp->get_cap_func_type();
            if(capFunc == nullptr) {
                linker.error("the lambda function type is not capturing", lambVal);
            }
        }

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
            mark_moved_value(linker.allocator, identifier, captured->linked->known_type(), linker, false);
        }

        lambVal->setIsCapturing(true);

    }

    // verify has return
    if(returnType->canonical()->kind() != BaseTypeKind::Void) {
        verify_has_return(linker, lambVal->scope, lambVal->encoded_location());
    }

    // restore the previous function type
    linker.current_func_type = prev_func_type;

    // now that linking has been performed of the lambda
    // moved ids and chains have been checked
    moved_identifiers.clear();
    moved_chains.clear();
    // we append the previously stored ids and chains
    moved_identifiers.insert(moved_identifiers.end(), prev_moved_ids.begin(), prev_moved_ids.end());
    moved_chains.insert(moved_chains.end(), prev_moved_chains.begin(), prev_moved_chains.end());

}

void SymResLinkBody::VisitNegativeValue(NegativeValue* negValue) {
    visit(negValue->getValue());
    // determine type for negative value
    negValue->determine_type(linker.comptime_scope.typeBuilder, linker.coreNodes, linker.implsIndex, linker);
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

void SymResLinkBody::VisitWrapValue(WrapValue* value) {
    visit(value->underlying);
    const auto runtimeType = value->getType()->as_runtime_type_unsafe();
    runtimeType->underlying = value->underlying->getType();
}

void SymResLinkBody::VisitNotValue(NotValue* value) {
    visit(value->getValue());
    // determine the type of not value
    value->determine_type(linker, linker.coreNodes, linker.implsIndex);
}

void SymResLinkBody::VisitBitwiseNot(BitwiseNot* value) {
    visit(value->getValue());
    // determine the type of bitwise not value
    value->determine_type(linker, linker.coreNodes, linker.implsIndex);
}

void SymResLinkBody::VisitPatternMatchExpr(PatternMatchExpr* expr) {
    // currently we emplace a void type
    // as expression is only used as a statement
    auto& typeBuilder = linker.comptime_scope.typeBuilder;
    expr->setType(typeBuilder.getVoidType());
    // linking pattern match expression
    const auto expression = expr->expression;
    auto& elseExpression = expr->elseExpression;
    auto& param_names = expr->param_names;
    // we use a reference to void type here, because user can dereference a pointer in the expression
    // we shouldn't error out when de-referencing a destructible struct (which we do)
    ReferenceType dummy_ref(typeBuilder.getVoidType(), !expr->is_const);
    visit(expression, &dummy_ref);
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
        value->setType(getErroredType());
        linker.error("expected a single value node for the switch value", value);
        return;
    }

    if(!value->stmt.attrs.operating_on_closed_value && !value->stmt.has_default_case()) {
        // we can check for closed enums and allow it some day
        linker.error("switch value must always have a default case", value);
    }


}

void SymResLinkBody::VisitLoopValue(LoopValue* value) {
    VisitLoopBlock(&value->stmt);
    // determine type of loop value
    const auto first = value->stmt.get_first_broken();
    if(first) {
        value->setType(first->getType());
    } else {
        value->setType(getErroredType());
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
            CHEM_THROW_RUNTIME("unknown");
#endif
        }
    }
}

bool isParentMethodOf(FunctionDeclaration* decl, StructValue* structVal) {
    // if(!decl->is_constructor_fn()) return false;
    const auto p = decl->parent();
    const auto other_p = structVal->linked_extendable();
    switch(p->kind()) {
        case ASTNodeKind::GenericStructDecl:
            return p->as_gen_struct_def_unsafe()->master_impl == other_p;
        case ASTNodeKind::GenericVariantDecl:
            return p->as_gen_variant_decl_unsafe()->master_impl == other_p;
        case ASTNodeKind::GenericUnionDecl:
            return p->as_gen_union_decl_unsafe()->master_impl == other_p;
        default:
            return p == other_p;
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
        const auto canon = exp_type->canonical();
        if(canon->kind() == BaseTypeKind::Reference) {
            structValue->setType(canon->as_reference_type_unsafe()->type);
        } else {
            structValue->setType(exp_type);
        }
    }
    if(!structValue->resolve_container(linker.genericInstantiator, !linker.generic_context)) {
        return;
    }
    structValue->diagnose_missing_members_for_init(linker);
    if(!structValue->allows_direct_init()) {
        const auto curr_func = linker.current_func_type->as_function();
        if(curr_func == nullptr || !isParentMethodOf(curr_func, structValue)) {
            linker.error(structValue) << "struct with name '" << structValue->linked_extendable()->name_view() << "' has a constructor, use @direct_init to allow direct initialization";
        }
    }
    auto refTypeKind = structValue->getRefType()->kind();
    if(refTypeKind == BaseTypeKind::Generic) {
        for (auto& arg: structValue->generic_list()) {
            visit(arg);
        }
    }
    // linking values
    for (auto &val: structValue->values) {
        auto& val_ptr = val.second.value;
        const auto value = val_ptr;
        auto child_node = structValue->linked_member_or_struct_of(val.first);
        if(!child_node) {
            linker.error(value) << "unresolved child '" << val.first << "' in struct declaration";
            continue;
        }
        auto child_type = child_node->known_type();
        visit(val_ptr, child_type);
        const auto member = structValue->direct_variable(val.first);
        if(member) {
            const auto mem_type = member->known_type();
            mark_moved_value(linker.allocator, val.second.value, mem_type, linker);
            auto implicit = mem_type->implicit_constructor_for(val_ptr);
            if(implicit) {
                link_with_implicit_constructor(*this, implicit, val_ptr);
            }
        }
    }
}

void SymResLinkBody::VisitExpressiveString(ExpressiveString* value) {
    for(auto& val : value->values) {
        visit(val);
    }
}

void SymResLinkBody::VisitZeroedValue(ZeroedValue* value) {

    auto type = value->getType();
    if(type == nullptr) {
        if(expected_type == nullptr) {
            linker.error(value) << "couldn't infer type for zeroed value";
            return;
        }
        type = expected_type;
    }

    visit(type, value->type_location);

    // typechecking code (better move there)
    if(value->is_unsafe) return;
    const auto can_type = type->canonical();
    const auto linked = can_type->get_direct_linked_node();
    if(linked) {
        const auto container = linked->get_master_members_container();
        if (container) {
            if(container->allow_zeroed) return;
            bool has_ctor_or_dtor = container->get_first_user_defined_constructor() != nullptr || container->has_destructor();
            if (has_ctor_or_dtor) {
                linker.error(value) << "type '" << type->representation() << "' has a constructor or destructor, " << "zero-initialization is not allowed unless marked with '@allow_zeroed' or use 'zeroed:unsafe'";
            }
        }
    }
}

void SymResLinkBody::VisitDynamicValue(DynamicValue* value) {
    visit(value->getType());
    visit(value->value);
    // checking if the interface can be used with dynamic value
    const auto node = value->getType()->referenced->get_direct_linked_canonical_node();
    if(node->kind() == ASTNodeKind::InterfaceDecl) {
        const auto interface = node->as_interface_def_unsafe();
        if(interface->is_non_dynamic()) {
            linker.error(value) << "interface with name '" << interface->name_view() << "' is set to non-dynamic explicitly, it cannot be used dynamically";
            linker.warn(interface) << "non dynamic interface being used with dynamic value";
        }
        if(!interface->is_object_safe()) {
            linker.error(value) << "interface with name '" << interface->name_view() << "' is not object safe and therefore cannot be used with dynamic value";
            linker.warn(interface) << "non object-safe interface being used with dynamic value";
        }
        // TODO: must verify that an implementation exists
    }

}

// -----------------------------------------
// ------------ Movement API ---------------
// -----------------------------------------

// first chain is contained in other chain
// other chain's value size is bigger or equal to first chain
bool first_chain_is_contained_in(AccessChain& first, AccessChain& other_ptr) {
    unsigned j = 0;
    for(auto& value_ptr : first.values) {
        auto& value = *value_ptr;
        auto& other = *other_ptr.values[j];
        if(!value.is_equal(&other)) {
            return false;
        }
        j++;
    }
    return true;
}

bool SymResLinkBody::un_move_chain(AccessChain* chain_ptr) {
    if(moved_chains.empty()) return false;
    auto& chain = *chain_ptr;
    if(chain.values.size() == 1) {
        auto id = chain.values[0]->as_identifier();
        if(id && un_move_exact_id(id)) {
            return true;
        }
    }
    for(auto it = moved_chains.begin(); it != moved_chains.end(); ++it) {
        auto& moved = **it;
        if(moved.values.size() >= chain.values.size() && first_chain_is_contained_in(chain, moved)) {
            // Erase the element at the iterator position
            it = moved_chains.erase(it);
            return true;
        }
    }
    return false;
}

VariableIdentifier* SymResLinkBody::find_moved_id(VariableIdentifier* id) {
    for(auto& moved : moved_identifiers) {
        if(moved->linked == id->linked) {
            return moved;
        }
    }
    return nullptr;
}

bool SymResLinkBody::un_move_exact_id(VariableIdentifier* id) {
    if(moved_identifiers.empty()) return false;
    unsigned i = 0;
    for(auto& moved : moved_identifiers) {
        if(moved->linked == id->linked) {
            moved_identifiers.erase(moved_identifiers.begin() + i);
            return true;
        }
        i++;
    }
    return false;
}

bool SymResLinkBody::un_move_chain_with_first_id(VariableIdentifier* id) {
    unsigned i = 0;
    for(auto& moved : moved_chains) {
        auto& chain = *moved;
        auto& first_value = *chain.values[0];
        auto moved_id = first_value.as_identifier();
        if(moved_id && moved_id->linked == id->linked) {
            moved_chains.erase(moved_chains.begin() + i);
            return true;
        }
        i++;
    }
    return false;
}

bool SymResLinkBody::un_move_id(VariableIdentifier* id) {
    return un_move_exact_id(id) || un_move_chain_with_first_id(id);
}

// for example when consider_nested_members and consider_last_member are true:
// for given 'm' if only 'm.x' has been moved, we return it (nested members considered)
// for given 'm.x' if only 'm' has been moved, we return it (parent member considered)
// for given 'm.x' if only 'm.y' has been moved, we return null (unrelated not considered)
// for given 'm.x' if only 'm.x' has been moved, we return it (last member considered)
//
// for example when consider_nested_members and consider_last_member are false:
// for given 'm.x' if only 'm.x.y' has been moved, we return null (nested members not considered)
// for given 'm.x' if only 'm' has been moved, we return it (parent member being considered)
// for given 'm.x' if only 'm.y' has been moved, we return null (unrelated nor considered)
// for given 'm.x' if only 'm.x' has been moved, we return null (last member not considered)
AccessChain* SymResLinkBody::find_partially_matching_moved_chain(AccessChain& chain, bool consider_nested_members, bool consider_last_member) {
    auto first_value = chain.values[0];
    const auto first_value_kind = first_value->val_kind();
    AccessChain* smallest = nullptr;
    for(auto& moved_chain_ptr : moved_chains) {
        auto& moved_chain = *moved_chain_ptr;
        const auto moved_size = moved_chain.values.size();
        // since finding the smallest moved chain that matches with the given chain
        if(smallest && smallest->values.size() < moved_size) {
            continue;
        }
        auto& moved_chain_first = moved_chain.values[0];
        if(first_value->is_equal(moved_chain_first, first_value_kind, moved_chain_first->val_kind())) {
            const auto given_size = chain.values.size();
            // check for nested members or not ?
            if(!consider_nested_members && moved_size > given_size) continue;
            // check for last member
            if(!consider_last_member && moved_size == given_size) continue;
            auto matching = true;
            const auto less_size = std::min(moved_size, consider_last_member ? given_size : given_size - 1);
            unsigned i = 1; // zero has already been checked
            while(i < less_size) {
                if(!moved_chain.values[i]->is_equal(chain.values[i])) {
                    matching = false;
                    break;
                }
                i++;
            }
            if(matching) {
                smallest = moved_chain_ptr;
            }
        }
    }
    return smallest;
}

AccessChain* SymResLinkBody::find_smallest_moved_access_chain(VariableIdentifier* id) {
    AccessChain* smallest = nullptr;
    for(auto& chain : moved_chains) {
        auto& moved_chain = *chain;
        if(smallest && smallest->values.size() < moved_chain.values.size()) {
            continue;
        }
        auto& other_first = *moved_chain.values[0];
        if(id->is_equal(&other_first, ValueKind::Identifier, other_first.val_kind())) {
            smallest = chain;
        }
    }
    return smallest;
}

AccessChain* SymResLinkBody::find_moved_access_chain(VariableIdentifier* id) {
    for(auto& chain : moved_chains) {
        auto& moved_chain = *chain;
        auto& other_first = *moved_chain.values[0];
        if(id->is_equal(&other_first, ValueKind::Identifier, other_first.val_kind())) {
            return chain;
        }
    }
    return nullptr;
}

Value* SymResLinkBody::find_moved_chain_value(VariableIdentifier* id) {
    auto found = find_moved_id(id);
    if(found) return found;
    return find_smallest_moved_access_chain(id);
}

Value* SymResLinkBody::find_moved_chain_value(AccessChain* chain_ptr) {
    auto& chain = *chain_ptr;
    auto& first_value = *chain.values[0];
    const auto first_id = first_value.as_identifier();
    if(first_id) {
        if(chain.values.size() == 1) {
            return find_moved_chain_value(first_id);
        } else {
            auto found = find_moved_id(first_id);
            if(found) return found;
        }
    }
    return find_partially_matching_moved_chain(chain, true, true);
}

void SymResLinkBody::mark_moved_no_check(AccessChain* chain) {
    if(chain->values.size() == 1 && chain->values[0]->val_kind() == ValueKind::Identifier) {
        moved_identifiers.emplace_back(chain->values[0]->as_identifier());
    } else {
        moved_chains.emplace_back(chain);
    }
    chain->set_is_moved(true);
}

void SymResLinkBody::mark_moved_no_check(VariableIdentifier* id) {
    moved_identifiers.emplace_back(id);
    id->is_moved = true;
}

bool SymResLinkBody::check_chain(AccessChain* chain, bool assigning, ASTDiagnoser& diagnoser) {
    Value* moved;
    if(assigning) {
        moved = find_partially_matching_moved_chain(*chain, false, false);
    } else {
        moved = find_moved_chain_value(chain);
    }
    if(moved) {
        auto& diag = diagnoser.error((ASTNode*) chain);
        diag << "cannot " << (assigning ? "assign" : "access") << " \'" << chain->chain_representation() << "' as '" << moved->representation() << "' has been moved";
        auto message = diag.message;
        diagnoser.error((ASTNode*) moved) << message;
        return false;
    }
    return true;
}

bool SymResLinkBody::check_id(VariableIdentifier* id, ASTDiagnoser& diagnoser) {
    const auto moved = find_moved_chain_value(id);
    if(moved) {
        auto& diag = diagnoser.error(id);
        diag << "cannot access identifier '" << id->representation() << "' as '" << moved->representation() << "' has been moved";
        auto message = diag.message;
        diagnoser.error(moved) << message;
        return false;
    }
    return true;
}

bool SymResLinkBody::mark_moved_id(VariableIdentifier* id, ASTDiagnoser& diagnoser) {
    const auto moved = find_moved_chain_value(id);
    if(moved) {
        auto& diag = diagnoser.error(id);
        diag << "cannot move '" << id->representation() << "' as '" << moved->representation() << "' has been moved";
        auto message = diag.message;
        diagnoser.error(moved) << message;
        return false;
    }
    mark_moved_no_check(id);
    return true;
}

bool SymResLinkBody::mark_moved_value(Value* value, ASTDiagnoser& diagnoser) {
    const auto chain = value->as_access_chain();
    if(chain) {
        if(chain->values.size() == 1) {
            auto id = chain->values.back()->as_identifier();
            if(id) {
                auto did = mark_moved_id(id, diagnoser);
                if(did) {
                    chain->set_is_moved(true);
                }
                return did;
            }
        }
        const auto moved = find_moved_chain_value(chain);
        if(moved) {
            auto& diag = diagnoser.error((ASTNode*) chain);
            diag << "cannot move '" << chain->chain_representation() << "' as '" << moved->representation() << "' has been moved";
            auto message = diag.message;
            diagnoser.error((ASTNode*) moved) << message;
            return false;
        }
        mark_moved_no_check(chain);
        return true;
    } else {
        const auto id = value->as_identifier();
        if(id) {
            return mark_moved_id(id, diagnoser);
        }
    }
    return false;
}

bool is_func_call(Value* value) {
    switch(value->kind()) {
        case ValueKind::FunctionCall:
            return true;
        case ValueKind::AccessChain:
            return is_func_call(value->as_access_chain_unsafe()->values.back());
        default:
            return false;
    }
}

bool SymResLinkBody::is_value_movable(Value* value_ptr, BaseType* type) {
    auto& value = *value_ptr;
    const auto canonical = type->canonical();
    switch(canonical->kind()) {
        case BaseTypeKind::Reference:
            return false;
        case BaseTypeKind::CapturingFunction:
            return true;
        default:
            break;
    }
    if(is_func_call(value_ptr)) {
        return false;
    }
    const auto linked_def = type->get_direct_linked_struct();
    if(linked_def && linked_def->MembersContainer::requires_moving()) {
        return true;
    }
    return false;
}

bool is_generic_instantiation(ASTNode* gen_node, ASTNode* inst_node) {
    // checking if inst node is one of its instantiations
    switch(gen_node->kind()) {
        case ASTNodeKind::GenericStructDecl: {
            const auto gen_decl = gen_node->as_gen_struct_def_unsafe();
            for (const auto inst: gen_decl->instantiations) {
                if (inst == inst_node) {
                    return true;
                }
            }
            break;
        }
        case ASTNodeKind::GenericUnionDecl: {
            const auto gen_decl = gen_node->as_gen_union_decl_unsafe();
            for (const auto inst: gen_decl->instantiations) {
                if (inst == inst_node) {
                    return true;
                }
            }
            break;
        }
        case ASTNodeKind::GenericVariantDecl: {
            const auto gen_decl = gen_node->as_gen_variant_decl_unsafe();
            for (const auto inst: gen_decl->instantiations) {
                if (inst == inst_node) {
                    return true;
                }
            }
            break;
        }
        default:
            return false;
    }
    return false;
}

bool is_movable(VariableIdentifier* id) {
    switch(id->linked->kind()) {
        case ASTNodeKind::StructMember:
        case ASTNodeKind::PatternMatchId:
        case ASTNodeKind::VariantCaseVariable:
            return false;
        default:
            return true;
    }
}

bool SymResLinkBody::mark_moved_value(
        ASTAllocator& allocator,
        Value* value_ptr,
        BaseType* expected_type_non_canon,
        ASTDiagnoser& diagnoser,
        bool check_implicit_constructors
) {
    auto& value = *value_ptr;
    switch(value.kind()) {
        case ValueKind::AccessChain:
            if(value.as_access_chain_unsafe()->values.back()->kind() == ValueKind::FunctionCall) {
                return false;
            }
            break;
        case ValueKind::FunctionCall:
            return false;
        case ValueKind::StructValue:
            return false;
        default:
            break;
    }
    const auto expected_type = expected_type_non_canon ? expected_type_non_canon->canonical() : nullptr;
    const auto expected_type_kind = expected_type ? expected_type->kind() : BaseTypeKind::Unknown;
    if (expected_type_kind == BaseTypeKind::Reference || expected_type_kind == BaseTypeKind::Pointer || expected_type_kind == BaseTypeKind::String || expected_type_kind == BaseTypeKind::Dynamic) {
        return false;
    }
    const auto createdType = value.getType();
    if(!createdType) return false;
    const auto type = createdType->canonical();
    const auto linked_node = type->get_direct_linked_node();
    if(!linked_node) {
        return false;
    }
    const auto linked_node_kind = linked_node->kind();
    // TODO this doesn't account for typealiases
    if(linked_node_kind == ASTNodeKind::GenericTypeParam) {
        if (!linked_node->as_generic_type_param_unsafe()->current_bits.has(InterfaceBits::COPY_BIT)) {
            switch(value.kind()) {
                case ValueKind::Identifier:
                    if(!is_movable(value.as_identifier_unsafe())) {
                        diagnoser.error("cannot move this value without re-initializing memory", &value);
                        return false;
                    }
                    break;
                case ValueKind::AccessChain: {
                    auto chain = value.as_access_chain_unsafe();
                    const auto last_value = chain->values.back();
                    if (last_value->kind() == ValueKind::Identifier && !is_movable(last_value->as_identifier_unsafe())) {
                        diagnoser.error("cannot move this value without re-initializing memory", &value);
                        return false;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return mark_moved_value(&value, diagnoser);
    }
    if(!ASTNode::isMembersContainer(linked_node_kind)) {
        return false;
    }
    const auto linked_def = linked_node->as_members_container_unsafe();
    if(linked_def->destructor_func() == nullptr) {
        return false;
    }
    bool final = false;
    if(expected_type) {
        const auto pure_expected = expected_type->pure_type(allocator);
        const auto pure_expected_kind = pure_expected->kind();
        const auto expected_node = pure_expected->get_ref_or_linked_node();
        if(!expected_node) {
            if(expected_type_kind != BaseTypeKind::Any) {
                diagnoser.error("cannot move a struct to a non struct type", &value);
            }
            return false;
        }
        switch(value.kind()) {
            case ValueKind::Identifier:
                if(!is_movable(value.as_identifier_unsafe())) {
                    diagnoser.error("cannot move this value without re-initializing memory", &value);
                    return false;
                }
                break;
            case ValueKind::AccessChain: {
                auto chain = value.as_access_chain_unsafe();
                const auto last_value = chain->values.back();
                if (last_value->kind() == ValueKind::Identifier && !is_movable(last_value->as_identifier_unsafe())) {
                    diagnoser.error("cannot move this value without re-initializing memory", &value);
                    return false;
                }
                break;
            }
            default:
                break;
        }
        if(expected_node->kind() == ASTNodeKind::GenericTypeParam) {
            return mark_moved_value(&value, diagnoser);
        }
        if (expected_node == (ASTNode*) linked_def) {
            final = mark_moved_value(&value, diagnoser);
        } else {
            if(is_generic_instantiation(expected_node, linked_def)) {
                final = mark_moved_value(&value, diagnoser);
            } else {
                const auto implicit = pure_expected->implicit_constructor_for(&value);
                if (implicit && check_implicit_constructors) {
                    auto& param_type = *implicit->params[0]->type->canonical();
                    if (!param_type.is_reference()) { // not a reference type (requires moving)
                        final = mark_moved_value(&value, diagnoser);
                    }
                } else {
                    diagnoser.error("unknown value being moved, where the struct types don't match", &value);
                    return false;
                }
            }
        }
    } else {
        final = mark_moved_value(&value, diagnoser);
    }
    if(final) {
        return true;
    }
    return false;
}

bool SymResLinkBody::mark_un_moved_lhs_value(Value* value_ptr, BaseType* value_type) {
    if(!value_type || !is_value_movable(value_ptr, value_type)) {
        return false;
    }
    auto& value = *value_ptr;
    switch(value.val_kind()) {
        case ValueKind::AccessChain:{
            const auto chain = value.as_access_chain_unsafe();
            if(chain->values.size() == 1 && chain->values.front()->kind() == ValueKind::Identifier) {
                const auto id = chain->values.back()->as_identifier_unsafe();
                if(un_move_id(id)) {
                    chain->set_is_moved(true);
                    id->is_moved = true;
                    return true;
                } else {
                    chain->set_is_moved(false);
                    id->is_moved = false;
                }
            } else {
                // we indicate if previous value should be destructed by setting lhs of assignment's is_moved to true or false
                // we set this to true, so assignment doesn't destruct before the store
                if (un_move_chain(chain)) {
                    // setting true, to indicate that value was moved before, and this should not be destructed
                    // we set this to true, so assignment doesn't destruct before the store
                    chain->set_is_moved(true);
                    return true;
                } else {
                    // setting false, to indicate that value is not moved, and this should be destructed
                    chain->set_is_moved(false);
                }
            }
            break;
        }
        case ValueKind::Identifier: {
            const auto id = value.as_identifier_unsafe();
            // we indicate if previous value should be destructed by setting lhs of assignment's is_moved to true or false
            // we set this to true, so assignment doesn't destruct before the store
            if(un_move_id(id)) {
                // setting true, to indicate that value was moved before, and this should not be destructed
                // we set this to true, so assignment doesn't destruct before the store
                id->is_moved = true;
                return true;
            } else {
                // setting false, to indicate that value is not moved, and this should be destructed
                id->is_moved = false;
            }
            break;
        }
        default:
            return false;
    }
    return false;
}