// Copyright (c) Chemical Language Foundation 2025.

#include "ast/base/InterpretScope.h"
#include "ast/base/ASTNode.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/Break.h"
#include "ast/statements/Continue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/types/FunctionType.h"
#include "ast/structures/ForLoop.h"
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
    if(ASTNode::isLoopASTNode(node->kind())) {
        const auto loop_node = node->as_loop_node_unsafe();
        loop_node->body.stopInterpretOnce();
        loop_node->stopInterpretation();
        return;
    }
    const auto parent = node->parent();
    if(parent) {
        stop_interpretation_above_once(parent);
    }
}

void skip_interpretation_above_once(ASTNode* node) {
    if(ASTNode::isLoopASTNode(node->kind())) {
        node->as_loop_node_unsafe()->body.stopInterpretOnce();
        return;
    }
    const auto parent = node->parent();
    if(parent) {
        skip_interpretation_above_once(parent);
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
        child.interpret(&loop->body);
        if (loop->stoppedInterpretation) {
            loop->stoppedInterpretation = false;
            break;
        }
        child.interpret(loop->incrementerExpr);
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
    unsigned i = 0;
    const auto size = stmt->scopes.size();
    while(i < size) {
        for(auto& casePair : stmt->cases) {
            if(casePair.second == i && casePair.first) {
                auto eval_first = casePair.first->evaluated_value(scope);
                const auto isEqualEval = scope.evaluate(Operation::IsEqual, eval_first, cond, ZERO_LOC, casePair.first);
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

void interpret(InterpretScope& scope, SwitchStatement* stmt) {
    const auto body = eval_switch_stmt_block(scope, stmt);
    if(body) {
        scope.interpret(body);
    }
}

Value* evaluated_value(InterpretScope &scope, SwitchStatement* stmt) {
    const auto body = eval_switch_stmt_block(scope, stmt);
    if(body) {
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
    node->value.evaluated_value(scope);
}

inline void interpret(InterpretScope& scope, PlacementNewNode* node) {
    node->value.evaluated_value(scope);
}

void interpret(InterpretScope& scope, VarInitStatement* stmt) {
    if (stmt->value) {
        auto initializer = stmt->value->scope_value(scope);
        scope.declare(stmt->name_view(), initializer);
    }
    stmt->decl_scope = &scope;
}

void interpret(InterpretScope& scope, DoWhileLoop* loop) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    do {
        child.interpret(&loop->body);
        if (loop->stoppedInterpretation) {
            loop->stoppedInterpretation = false;
            break;
        }
    } while (loop->condition->evaluated_bool(child));
}

void interpret(InterpretScope& scope, WhileLoop* loop) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    while (loop->condition->evaluated_bool(child)) {
        child.interpret(&loop->body);
        if (loop->stoppedInterpretation) {
            loop->stoppedInterpretation = false;
            break;
        }
    }
}

void interpret(InterpretScope& scope, std::vector<ASTNode*>& nodes, bool& stoppedInterpretOnce) {
    for (const auto &node: nodes) {
        scope.interpret(node);
        if (stoppedInterpretOnce) {
            stoppedInterpretOnce = false;
            return;
        }
    }
}

inline void interpret(InterpretScope& scope, Scope* body) {
    interpret(scope, body->nodes, body->stoppedInterpretOnce);
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
        case ASTNodeKind::Scope:
            ::interpret(*this, node->as_scope_unsafe());
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