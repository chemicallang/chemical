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
#include "ast/statements/SwitchStatement.h"
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

void interpret(InterpretScope& scope, SwitchStatement* stmt) {
    const auto cond = stmt->expression->evaluated_value(scope);
    if(!cond) {
        scope.error("couldn't evaluate the expression", stmt->expression);
        return;
    }
    unsigned i = 0;
    const auto size = stmt->scopes.size();
    while(i < size) {
        for(auto& casePair : stmt->cases) {
            if(casePair.second == i && casePair.first) {
                auto eval_first = casePair.first->evaluated_value(scope);
                const auto isEqualEval = scope.evaluate(Operation::IsEqual, eval_first, cond, ZERO_LOC, casePair.first);
                if(isEqualEval->val_kind() == ValueKind::Bool && isEqualEval->get_the_bool()) {
                    auto& body = stmt->scopes[i];
                    scope.interpret(&body);
                    return;
                }
            }
        }
        i++;
    }
    if(stmt->has_default_case()) {
        auto& body = stmt->scopes[stmt->defScopeInd];
        scope.interpret(&body);
        return;
    }
}

void interpret(InterpretScope& scope, IfStatement* stmt) {
    if (stmt->condition->evaluated_bool(scope)) {
        InterpretScope child(&scope, scope.allocator, scope.global);
        child.interpret(&stmt->ifBody);
    } else {
        for (auto const &elseIf: stmt->elseIfs) {
            if (elseIf.first->evaluated_bool(scope)) {
                InterpretScope child(&scope, scope.allocator, scope.global);
                child.interpret(const_cast<Scope *>(&elseIf.second));
                return;
            }
        }
        if (stmt->elseBody.has_value()) {
            InterpretScope child(&scope, scope.allocator, scope.global);
            child.interpret(&stmt->elseBody.value());
        }
    }
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

void interpret(InterpretScope& scope, Scope* body) {
    for (const auto &node: body->nodes) {
        scope.interpret(node);
        if (body->stoppedInterpretOnce) {
            body->stoppedInterpretOnce = false;
            return;
        }
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