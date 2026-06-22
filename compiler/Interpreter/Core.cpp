// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/InterpretScope.h"
#include "ast/base/TypeBuilder.h"
#include "ast/base/ASTNode.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/ForInLoop.h"
#include "ast/structures/BlockScope.h"
#include "ast/values/IntNumValue.h"
#include "ast/values/StringValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/types/LinkedType.h"
#include "ast/values/PointerValue.h"
#include "ast/types/PointerType.h"
#include "ast/values/BoolValue.h"
#include "ast/values/FloatValue.h"
#include "ast/values/DoubleValue.h"
#include "ast/structures/StructDefinition.h"
#include "ast/statements/Typealias.h"
#include "ast/statements/Return.h"
#include "ast/values/SwitchValue.h"
#include "ast/values/IfValue.h"
#include "ast/values/ValueNode.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/IncDecNode.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/statements/PlacementNewNode.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/If.h"
#include "ast/base/LoopASTNode.h"
#include "ast/values/VariantCase.h"
#include "ast/values/VariantCaseVariable.h"
#include "ast/values/StructValue.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/statements/ProvideStmt.h"
#include "ast/structures/LoopBlock.h"


void stop_interpretation_above(ASTNode* node) {
    if(ASTNode::isLoopASTNode(node->kind())) {
        node->as_loop_node_unsafe()->stopInterpretation();
    }
    const auto parent = node->parent();
    if(parent) {
        stop_interpretation_above(parent);
    }
}

void stop_interpretation_above_once(ASTNode* node) {
    const auto loop_node = node->get_loop_node_above();
    if(loop_node) {
        loop_node->body.stopInterpretOnce();
        loop_node->stopInterpretation();
    }
}

void skip_interpretation_above_once(ASTNode* node) {
    const auto loop_node = node->get_loop_node_above();
    if(loop_node) {
        loop_node->body.stopInterpretOnce();
    }
}

Value* evaluate(InterpretScope& scope, Scope* body);

inline void interpret(InterpretScope& scope, BreakStatement* stmt) {
    stop_interpretation_above_once(stmt->parent());
}

inline void interpret(InterpretScope& scope, ContinueStatement* stmt) {
    skip_interpretation_above_once(stmt->parent());
}

inline void interpret(InterpretScope& scope, AssignStatement* assign) {
    assign->lhs->set_value(scope, assign->value, assign->assOp, assign->encoded_location());
}

void interpret(InterpretScope& scope, ForLoop* loop) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    child.interpret(loop->initializer);
    while (loop->conditionExpr->evaluated_bool(child)) {
        {
            InterpretScope body_scope(&child, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop->stoppedInterpretation) {
            loop->stoppedInterpretation = false;
            break;
        }
        child.interpret(loop->incrementerExpr);
    }
}

void interpret(InterpretScope& scope, ForInLoop* loop) {
    const auto evalExpr = loop->expr->evaluated_value(scope);
    if(!evalExpr) {
        scope.error("couldn't evaluate for-in expression", loop->expr);
        return;
    }

    const auto kind = evalExpr->val_kind();

    if(kind == ValueKind::String) {
        const auto strVal = evalExpr->as_string_unsafe();
        const auto& str = strVal->value;

        for(uint64_t i = 0; i < str.size(); i++) {
            InterpretScope child(&scope, scope.allocator, scope.global);

            // declare the loop variable (char)
            auto charVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                (uint64_t)(unsigned char)str[i],
                scope.global->typeBuilder.getCharType(),
                ZERO_LOC
            );
            child.declare(loop->id, charVal);

            // declare the index variable if present
            if(loop->index_init) {
                auto indexVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                    i,
                    (IntNType*) (BaseType*) loop->index_init->type,
                    ZERO_LOC
                );
                child.declare(loop->index_init->name_view(), indexVal);
            }

            child.interpret(&loop->body);

            if(loop->attrs.stoppedInterpretation) {
                loop->attrs.stoppedInterpretation = false;
                break;
            }
        }
    } else if(kind == ValueKind::ArrayValue) {
        const auto arrVal = evalExpr->as_array_value_unsafe();

        for(uint64_t i = 0; i < arrVal->values.size(); i++) {
            InterpretScope child(&scope, scope.allocator, scope.global);

            const auto elemVal = arrVal->values[i]->evaluated_value(scope);
            if(elemVal) {
                child.declare(loop->id, elemVal);
            }

            // declare the index variable if present
            if(loop->index_init) {
                auto indexVal = new (scope.allocate<IntNumValue>()) IntNumValue(
                    i,
                    (IntNType*) (BaseType*) loop->index_init->type,
                    ZERO_LOC
                );
                child.declare(loop->index_init->name_view(), indexVal);
            }

            child.interpret(&loop->body);

            if(loop->attrs.stoppedInterpretation) {
                loop->attrs.stoppedInterpretation = false;
                break;
            }
        }
    } else {
        scope.error("for-in loop not supported for expression type (kind: " + std::to_string(static_cast<int>(kind)) + ") in comptime", loop->expr);
    }
}

void interpret(InterpretScope& scope, ReturnStatement* stmt) {
    scope.global->current_func_type->set_return(scope, stmt->value);
    stop_interpretation_above(stmt->parent());
}

Scope* eval_switch_stmt_block(InterpretScope& scope, SwitchStatement* stmt) {
    const auto cond = stmt->expression->evaluated_value(scope);
    if(!cond) {
        scope.error("couldn't evaluate the expression", stmt->expression);
        return nullptr;
    }

    // Check if this is a variant switch — match by member index instead of value equality.
    const auto variantDef = stmt->getVarDefFromExpr();
    if(variantDef) {
        // Determine the active member index from the condition (the variant struct value).
        int condMemberIndex = -1;
        if(cond->val_kind() == ValueKind::StructValue) {
            auto structVal = cond->as_struct_value_unsafe();
            auto it = scope.global->variant_member_index_map.find(structVal);
            if(it != scope.global->variant_member_index_map.end()) {
                condMemberIndex = (int)it->second;
            }
        }
        if(condMemberIndex >= 0) {
            unsigned i = 0;
            const auto size = stmt->scopes.size();
            while(i < size) {
                for(auto& casePair : stmt->cases) {
                    if(casePair.second == i && casePair.first) {
                        // VariantCase carries the VariantMember pointer
                        if(casePair.first->val_kind() == ValueKind::VariantCase) {
                            auto varCase = casePair.first->as_variant_case_unsafe();
                            auto member = varCase->member;
                            if(member) {
                                int caseMemberIndex = (int)variantDef->direct_child_index(member->name_view());
                                if(caseMemberIndex == condMemberIndex) {
                                    return &stmt->scopes[i];
                                }
                            }
                        }
                    }
                }
                i++;
            }
            if(stmt->has_default_case()) {
                return &stmt->scopes[stmt->defScopeInd];
            }
            return nullptr;
        }
        // Fall through to normal matching if we couldn't determine the member index
    }

    unsigned i = 0;
    const auto size = stmt->scopes.size();
    while(i < size) {
        for(auto& casePair : stmt->cases) {
            if(casePair.second == i && casePair.first) {
                auto eval_first = casePair.first->evaluated_value(scope);
                const auto isEqualEval = scope.evaluate(Operation::IsEqual, eval_first, cond, ZERO_LOC, casePair.first);
                if(!isEqualEval || isEqualEval->val_kind() == ValueKind::NullValue) {
                    // evaluate returned null (unknown operation) — skip this case
                    continue;
                }
                if(isEqualEval->val_kind() == ValueKind::Bool && isEqualEval->get_the_bool()) {
                    return &stmt->scopes[i];
                }
            }
        }
        i++;
    }
    if(stmt->has_default_case()) {
        return &stmt->scopes[stmt->defScopeInd];
    }
    return nullptr;
}

/**
 * Helper: declare variant case variables from the matched variant case
 * into the given interpret scope. Returns the number of variables declared.
 */
static unsigned declare_variant_case_vars(
    InterpretScope& scope,
    SwitchStatement* stmt,
    Scope* body,
    Value* cond
) {
    if(!cond || cond->val_kind() != ValueKind::StructValue) return 0;
    auto condStruct = cond->as_struct_value_unsafe();
    const auto variantDef = stmt->getVarDefFromExpr();
    if(!variantDef) return 0;
    unsigned count = 0;
    for(auto& casePair : stmt->cases) {
        if(casePair.first && casePair.first->val_kind() == ValueKind::VariantCase) {
            if(&stmt->scopes[casePair.second] == body) {
                auto varCase = casePair.first->as_variant_case_unsafe();
                for(auto& caseVar : varCase->identifier_list) {
                    auto found = condStruct->values.find(caseVar->name);
                    if(found != condStruct->values.end() && found->second.value) {
                        scope.declare(caseVar->name, found->second.value);
                        count++;
                    }
                }
                break;
            }
        }
    }
    return count;
}

void interpret(InterpretScope& scope, SwitchStatement* stmt) {
    const auto body = eval_switch_stmt_block(scope, stmt);
    if(body) {
        // Use a child scope for variant switch cases to keep variable declarations clean
        InterpretScope child(&scope, scope.allocator, scope.global);
        if(!scope.global->variant_member_index_map.empty()) {
            const auto cond = stmt->expression->evaluated_value(scope);
            declare_variant_case_vars(child, stmt, body, cond);
        }
        child.interpret(body);
    }
}

Value* evaluated_value(InterpretScope &scope, SwitchStatement* stmt) {
    const auto body = eval_switch_stmt_block(scope, stmt);
    if(body) {
        // For variant switches, need to declare case variables before evaluating
        if(!scope.global->variant_member_index_map.empty()) {
            const auto cond = stmt->expression->evaluated_value(scope);
            declare_variant_case_vars(scope, stmt, body, cond);
        }
        return evaluate(scope, body);
    }
    return scope.getNullValue();
}

Scope* eval_if_stmt_block(InterpretScope& scope, IfStatement* stmt) {
    if (stmt->condition->evaluated_bool(scope)) {
        return &stmt->ifBody;
    } else {
        for (auto const &elseIf: stmt->elseIfs) {
            if (elseIf.first->evaluated_bool(scope)) {
                return const_cast<Scope *>(&elseIf.second);
            }
        }
        if (stmt->elseBody.has_value()) {
            return &stmt->elseBody.value();
        }
    }
    return nullptr;
}

void interpret(InterpretScope& scope, IfStatement* stmt) {
    const auto block = eval_if_stmt_block(scope, stmt);
    if(block) {
        InterpretScope child(&scope, scope.allocator, scope.global);
        child.interpret(block);
    }
}

Value* evaluated_value(InterpretScope &scope, IfStatement* stmt) {
    const auto body = eval_if_stmt_block(scope, stmt);
    if(body) {
        return evaluate(scope, body);
    }
    return scope.getNullValue();
}

inline void interpret(InterpretScope& scope, ValueWrapperNode* node) {
    node->value->evaluated_value(scope);
}

inline void interpret(InterpretScope& scope, AccessChainNode* node) {
    node->chain.evaluated_value(scope);
}

inline void interpret(InterpretScope& scope, IncDecNode* node) {
    node->value.evaluated_value(scope);
}

inline void interpret(InterpretScope& scope, PatternMatchExprNode* node) {
    // Evaluate the pattern match expression to check the variant type
    // For now, the interpreter always takes the else branch (no variant type checking yet)
    // This handles break/continue/return/defValue for pattern matching
    const auto& elseExpr = node->value.elseExpression;
    switch(elseExpr.kind) {
        case PatternElseExprKind::Break:
            stop_interpretation_above_once(node);
            break;
        case PatternElseExprKind::Continue:
            skip_interpretation_above_once(node);
            break;
        case PatternElseExprKind::Return: {
            if(elseExpr.value) {
                scope.global->current_func_type->set_return(scope, elseExpr.value);
            }
            stop_interpretation_above(node->parent());
            break;
        }
        case PatternElseExprKind::DefValue: {
            if(elseExpr.value && !node->value.param_names.empty()) {
                const auto evalValue = elseExpr.value->evaluated_value(scope);
                if(evalValue) {
                    scope.declare(node->value.param_names[0]->identifier, evalValue);
                }
            }
            break;
        }
        default:
            // No else expression - just evaluate normally
            node->value.evaluated_value(scope);
            break;
    }
}

inline void interpret(InterpretScope& scope, PlacementNewNode* node) {
    node->value.evaluated_value(scope);
}

void interpret(InterpretScope& scope, LoopBlock* loop) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    while (true) {
        {
            InterpretScope body_scope(&child, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop->stoppedInterpretation) {
            loop->stoppedInterpretation = false;
            break;
        }
    }
}

void interpret(InterpretScope& scope, ProvideStmt* stmt) {
    // Evaluate the provide value
    auto val = stmt->value->evaluated_value(scope);
    if(val) {
        // Store the implicit arg in the current scope's map.
        // The provide body is interpreted in-place (same scope, no new scope),
        // so the implicit arg is available for any function called within.
        scope.implicit_args[stmt->identifier] = val;
    }
    scope.interpret(&stmt->body);
    // Clean up
    scope.implicit_args.erase(stmt->identifier);
}

void interpret(InterpretScope& scope, VarInitStatement* stmt) {
    if (stmt->value) {
        auto initializer = stmt->value->scope_value(scope);
        scope.declare(stmt->name_view(), initializer);
        // Handle move semantics: if the initializer is a direct variable reference
        // to a non-copyable struct, clear the source (move, not copy).
        if(stmt->value->val_kind() == ValueKind::Identifier) {
            auto id = stmt->value->as_identifier_unsafe();
            auto it = scope.find_value_iterator(id->value);
            if(it.first != it.second.values.end() && it.first->second != nullptr) {
                auto srcVal = it.first->second;
                if(srcVal->val_kind() == ValueKind::StructValue) {
                    auto ext = srcVal->as_struct_value_unsafe()->linked_extendable();
                    if(ext && ext->kind() == ASTNodeKind::StructDecl) {
                        auto sd = (StructDefinition*)ext;
                        if(sd->has_destructor()) {
                            // Move: clear the source value so it's not destructed
                            it.first->second = nullptr;
                        }
                    }
                }
            }
        }
    } else if (stmt->type) {
        // Uninitialized variable: create a default zero value based on type
        auto type = stmt->type;
        auto kind = type->kind();
        if (kind == BaseTypeKind::Array) {
            auto arrType = type->as_array_type_unsafe();
            auto size = arrType->get_array_size();
            if (size > 0) {
                auto arrVal = new (scope.allocate<ArrayValue>()) ArrayValue(stmt->encoded_location(), arrType);
                arrVal->explicit_size = (unsigned int)size;
                // evaluated_value will allocate zero-initialized contiguousData
                arrVal->evaluated_value(scope);
                scope.declare(stmt->name_view(), arrVal);
            }
        } else if (kind == BaseTypeKind::Linked) {
            auto linkedType = type->as_linked_type_unsafe();
            // Resolve typealiases to the underlying struct definition
            ASTNode* linked = linkedType->linked;
            if (linked && linked->kind() == ASTNodeKind::TypealiasStmt) {
                auto alias = (TypealiasStatement*) linked;
                if (alias->actual_type) {
                    auto actualKind = alias->actual_type->kind();
                    if (actualKind == BaseTypeKind::Linked) {
                        linked = alias->actual_type->as_linked_type_unsafe()->linked;
                    }
                }
            }
            if (linked && linked->kind() == ASTNodeKind::StructDecl) {
                auto structDef = (StructDefinition*) linked;
                auto structVal = new (scope.allocate<StructValue>()) StructValue(
                    stmt->type, structDef, structDef, stmt->encoded_location()
                );
                // Populate all fields with default/null values
                for (const auto field : structDef->variables()) {
                    auto defValue = field->default_value();
                    if (defValue) {
                        structVal->values.emplace(
                            field->name,
                            StructMemberInitializer { field->name, defValue->scope_value(scope) }
                        );
                    } else {
                        structVal->values.emplace(
                            field->name,
                            StructMemberInitializer { field->name, scope.getNullValue() }
                        );
                    }
                }
                scope.declare(stmt->name_view(), structVal);
            }
        } else if (kind == BaseTypeKind::IntN) {
            auto intType = type->as_intn_type_unsafe();
            auto val = new (scope.allocate<IntNumValue>()) IntNumValue(0, intType, stmt->encoded_location());
            scope.declare(stmt->name_view(), val);
        } else if (kind == BaseTypeKind::Pointer) {
            // Uninitialized pointer: declare as null pointer
            auto& typeBuilder = scope.global->typeBuilder;
            auto val = new (scope.allocate<PointerValue>()) PointerValue(
                nullptr, type->as_pointer_type_unsafe()->type, 0, 0, stmt->encoded_location()
            );
            scope.declare(stmt->name_view(), val);
        } else if (kind == BaseTypeKind::Bool) {
            auto& typeBuilder = scope.global->typeBuilder;
            auto val = new (scope.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), stmt->encoded_location());
            scope.declare(stmt->name_view(), val);
        }
    }
}

void interpret(InterpretScope& scope, DoWhileLoop* loop) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    do {
        {
            InterpretScope body_scope(&child, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop->stoppedInterpretation) {
            loop->stoppedInterpretation = false;
            break;
        }
    } while (loop->condition->evaluated_bool(child));
}

void interpret(InterpretScope& scope, WhileLoop* loop) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    while (loop->condition->evaluated_bool(child)) {
        {
            InterpretScope body_scope(&child, scope.allocator, scope.global);
            body_scope.interpret(&loop->body);
        } // body_scope destroyed here, calling destructors on iteration-local variables
        if (loop->stoppedInterpretation) {
            loop->stoppedInterpretation = false;
            break;
        }
    }
}

void interpret(InterpretScope& scope, std::vector<ASTNode*>& nodes, bool& stoppedInterpretOnce) {
    for (const auto &node: nodes) {
        scope.interpret(node);
        if (stoppedInterpretOnce || scope.stopInterpretation) {
            stoppedInterpretOnce = false;
            return;
        }
    }
}

inline void interpret(InterpretScope& scope, Scope* body) {
    interpret(scope, body->nodes, body->stoppedInterpretOnce);
}

inline void interpret(InterpretScope& scope, BlockScope* body) {
    // TODO: stoppedInterpretOnce flag should be removed from Scope
    // TODO: BlockScope never stops, even if a return happens in the body
    InterpretScope child_scope(&scope, scope.allocator, scope.global);
    bool stopped = false; // never stops
    interpret(child_scope, body->nodes, stopped);
}

Value* evaluate(InterpretScope& scope, Scope* body);

Value* evaluate(InterpretScope& scope, BlockScope* body) {
    auto& nodes = body->nodes;
    if(nodes.empty()) return scope.getNullValue();
    if(nodes.size() > 1) {
        for(unsigned i = 0; i < nodes.size() - 1; i++) {
            scope.interpret(nodes[i]);
        }
    }
    const auto last = nodes.back();
    switch(last->kind()) {
        case ASTNodeKind::ValueNode:
            return last->as_value_node_unsafe()->value->evaluated_value(scope);
        case ASTNodeKind::IfStmt:
            return evaluated_value(scope, last->as_if_stmt_unsafe());
        case ASTNodeKind::SwitchStmt:
            return evaluated_value(scope, last->as_switch_stmt_unsafe());
        case ASTNodeKind::Block:
            return evaluate(scope, last->as_block_scope_unsafe());
        default:
            scope.interpret(last);
            return scope.getNullValue();
    }
}

Value* evaluate(InterpretScope& scope, Scope* body) {
    auto& nodes = body->nodes;
    if(nodes.size() > 1) {
        auto start = body->nodes.data();
        // skip the last one
        const auto end = start + (nodes.size() - 1);
        while(start != end) {
            scope.interpret(*start);
            start++;
        }
    }
    // lets do the last one
    const auto last = nodes.back();
    switch(last->kind()) {
        case ASTNodeKind::ValueNode:
            return last->as_value_node_unsafe()->value->evaluated_value(scope);
        case ASTNodeKind::IfStmt:
            return evaluated_value(scope, last->as_if_stmt_unsafe());
        case ASTNodeKind::SwitchStmt:
            return evaluated_value(scope, last->as_switch_stmt_unsafe());
        case ASTNodeKind::Block:
            return evaluate(scope, last->as_block_scope_unsafe());
        default:
            scope.interpret(last);
            return scope.getNullValue();
    }
}

void InterpretScope::interpret(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::AssignmentStmt:
            ::interpret(*this, node->as_assignment_unsafe());
            break;
        case ASTNodeKind::BreakStmt:
            ::interpret(*this, node->as_break_stmt_unsafe());
            break;
        case ASTNodeKind::ContinueStmt:
            ::interpret(*this, node->as_continue_stmt_unsafe());
            break;
        case ASTNodeKind::ForLoopStmt:
            ::interpret(*this, node->as_for_loop_unsafe());
            break;
        case ASTNodeKind::ForInLoopStmt:
            ::interpret(*this, node->as_for_in_loop_unsafe());
            break;
        case ASTNodeKind::ReturnStmt:
            ::interpret(*this, node->as_return_unsafe());
            break;
        case ASTNodeKind::SwitchStmt:
            ::interpret(*this, node->as_switch_stmt_unsafe());
            break;
        case ASTNodeKind::IfStmt:
            ::interpret(*this, node->as_if_stmt_unsafe());
            break;
        case ASTNodeKind::ValueWrapper:
            ::interpret(*this, node->as_value_wrapper_unsafe());
            break;
        case ASTNodeKind::AccessChainNode:
            ::interpret(*this, node->as_access_chain_node_unsafe());
            break;
        case ASTNodeKind::IncDecNode:
            ::interpret(*this, node->as_inc_dec_node_unsafe());
            break;
        case ASTNodeKind::PatternMatchExprNode:
            ::interpret(*this, node->as_pattern_match_expr_node_unsafe());
            break;
        case ASTNodeKind::PlacementNewNode:
            ::interpret(*this, node->as_placement_new_node_unsafe());
            break;
        case ASTNodeKind::VarInitStmt:
            ::interpret(*this, node->as_var_init_unsafe());
            break;
        case ASTNodeKind::DoWhileLoopStmt:
            ::interpret(*this, node->as_do_while_loop_unsafe());
            break;
        case ASTNodeKind::WhileLoopStmt:
            ::interpret(*this, node->as_while_loop_unsafe());
            break;
        case ASTNodeKind::LoopBlock:
            ::interpret(*this, node->as_loop_block_unsafe());
            break;
        case ASTNodeKind::ProvideStmt:
            ::interpret(*this, node->as_provide_stmt_unsafe());
            break;
        case ASTNodeKind::Scope:
            ::interpret(*this, node->as_scope_unsafe());
            break;
        case ASTNodeKind::Block:
            ::interpret(*this, node->as_block_scope_unsafe());
            break;
        default:
            break;
    }
}

// ---------- values

Value* IfValue::evaluated_value(InterpretScope &scope) {
    return ::evaluated_value(scope, &stmt);
}

Value* SwitchValue::evaluated_value(InterpretScope &scope) {
    return ::evaluated_value(scope, &stmt);
}