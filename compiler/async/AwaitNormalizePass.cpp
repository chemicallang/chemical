// Copyright (c) Chemical Language Foundation 2026.

#include "compiler/async/AwaitNormalizePass.h"

#include "ast/base/ASTAllocator.h"
#include "ast/base/BaseType.h"
#include "ast/structures/Scope.h"
#include "ast/structures/BlockScope.h"
#include "ast/structures/If.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/ForInLoop.h"
#include "ast/structures/LoopBlock.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Return.h"
#include "ast/statements/Assignment.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/statements/AccessChainNode.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/values/AwaitExpression.h"
#include "ast/values/Expression.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/AccessChain.h"
#include "ast/values/CastedValue.h"
#include "ast/values/IndexOperator.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/ArrayValue.h"
#include "ast/values/ExpressiveString.h"
#include "ast/values/VariableIdentifier.h"

#include "preprocess/visitors/RecursiveVisitor.h"

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

/**
 * Rewriter that hoists every AwaitExpression into a fresh VarInitStatement
 * placed immediately before the statement that contains it.
 */
struct AwaitRewriter {

    ASTAllocator& allocator;
    unsigned& counter;

    // list of the current statement's hoisted temps, in evaluation order
    std::vector<ASTNode*>* pending = nullptr;

    // the AST node the hoisted temps belong to (so they are local, not top level)
    ASTNode* current_parent = nullptr;

    AwaitRewriter(ASTAllocator& allocator, unsigned& counter)
        : allocator(allocator), counter(counter) {}

    chem::string_view make_name() {
        std::string s = "__chx_await_" + std::to_string(counter++);
        // allocate_str does not register the buffer for ASTAny destruction
        char* buf = allocator.allocate_str(s.data(), s.size());
        return chem::string_view(buf, s.size());
    }

    // hoist an await expression into a VarInitStatement, returning the
    // identifier that reads the hoisted value
    Value* hoist(AwaitExpression* await) {
        auto result_type = await->await_result_type ? await->await_result_type : await->getType();
        const auto loc = await->encoded_location();
        const auto name = make_name();
        auto stmt = new (allocator.allocate<VarInitStatement>()) VarInitStatement(
            false, false, name, TypeLoc(result_type, loc), await, current_parent, loc
        );
        auto id = new (allocator.allocate<VariableIdentifier>())
            VariableIdentifier(name, result_type, loc);
        id->linked = stmt;
        if(pending != nullptr) {
            pending->push_back(stmt);
        }
        return id;
    }

    Value* rewrite(Value* value) {
        if(value == nullptr) return nullptr;
        if(value->kind() == ValueKind::AwaitExpr) {
            return hoist(value->as_await_expression_unsafe());
        }
        switch(value->kind()) {
            case ValueKind::Expression: {
                auto expr = value->as_expression_unsafe();
                expr->firstValue = rewrite(expr->firstValue);
                expr->secondValue = rewrite(expr->secondValue);
                return expr;
            }
            case ValueKind::FunctionCall: {
                auto call = value->as_func_call_unsafe();
                call->parent_val = rewrite(call->parent_val);
                for(auto& v : call->values) {
                    v = rewrite(v);
                }
                return call;
            }
            case ValueKind::AccessChain: {
                auto chain = value->as_access_chain_unsafe();
                for(auto& v : chain->values) {
                    v = rewrite(v);
                }
                return chain;
            }
            case ValueKind::CastedValue: {
                auto casted = value->as_casted_value_unsafe();
                casted->value = rewrite(casted->value);
                return casted;
            }
            case ValueKind::IndexOperator: {
                auto index = value->as_index_op_unsafe();
                index->parent_val = rewrite(index->parent_val);
                index->idx = rewrite(index->idx);
                return index;
            }
            case ValueKind::ComptimeValue: {
                auto comptime = (ComptimeValue*) value;
                comptime->setValue(rewrite(comptime->getValue()));
                return comptime;
            }
            case ValueKind::ArrayValue: {
                auto array = value->as_array_value_unsafe();
                for(auto& v : array->values) {
                    v = rewrite(v);
                }
                return array;
            }
            case ValueKind::ExpressiveString: {
                auto str = value->as_expressive_str_unsafe();
                for(auto& v : str->values) {
                    v = rewrite(v);
                }
                return str;
            }
            default:
                // No known await-bearing children; leave as-is. The bootstrap
                // backends still handle any nested await transparently.
                return value;
        }
    }

    /**
     * normalize the nodes of a scope, inserting hoisted statements in place
     */
    void rewrite_scope(Scope& scope) {
        auto* prev_parent = current_parent;
        current_parent = scope.parent();
        std::vector<ASTNode*> rebuilt;
        rebuilt.reserve(scope.nodes.size() + 4);
        for(auto node : scope.nodes) {
            std::vector<ASTNode*> hoisted;
            rewrite_stmt(node, hoisted);
            for(auto h : hoisted) {
                rebuilt.push_back(h);
            }
            rebuilt.push_back(node);
        }
        scope.nodes = std::move(rebuilt);
        current_parent = prev_parent;
    }

    void rewrite_stmt(ASTNode* node, std::vector<ASTNode*>& hoisted) {
        auto* prev = pending;
        pending = &hoisted;
        rewrite_stmt_impl(node, hoisted);
        pending = prev;
    }

    void rewrite_stmt_impl(ASTNode* node, std::vector<ASTNode*>& hoisted) {
        if(node == nullptr) return;
        switch(node->kind()) {
            case ASTNodeKind::VarInitStmt: {
                auto stmt = node->as_var_init_unsafe();
                // already normalized: `var x = await ...`
                if(stmt->value != nullptr && stmt->value->kind() == ValueKind::AwaitExpr) {
                    return;
                }
                stmt->value = rewrite(stmt->value);
                return;
            }
            case ASTNodeKind::ReturnStmt: {
                auto stmt = (ReturnStatement*) node;
                stmt->value = rewrite(stmt->value);
                return;
            }
            case ASTNodeKind::AssignmentStmt: {
                auto stmt = (AssignStatement*) node;
                stmt->lhs = rewrite(stmt->lhs);
                stmt->value = rewrite(stmt->value);
                return;
            }
            case ASTNodeKind::ValueWrapper: {
                auto stmt = (ValueWrapperNode*) node;
                stmt->value = rewrite(stmt->value);
                return;
            }
            case ASTNodeKind::AccessChainNode: {
                auto stmt = (AccessChainNode*) node;
                auto& values = stmt->chain.values;
                for(auto& v : values) {
                    v = rewrite(v);
                }
                return;
            }
            case ASTNodeKind::IfStmt: {
                auto stmt = (IfStatement*) node;
                stmt->condition = rewrite(stmt->condition);
                rewrite_scope(stmt->ifBody);
                for(auto& [cond, body] : stmt->elseIfs) {
                    cond = rewrite(cond);
                    rewrite_scope(body);
                }
                if(stmt->elseBody.has_value()) {
                    rewrite_scope(stmt->elseBody.value());
                }
                return;
            }
            // Loop headers (condition / increment / for-in range) are evaluated
            // on every iteration, so an await there must NOT be hoisted out of
            // the loop. We leave those awaits for the transparent bootstrap
            // backend; the loop-body statements are still normalized. Proper
            // loop-header handling (`while(await c)` -> `loop { t = await c;
            // if(!t) break; ... }`) lands with the coroutine lowering.
            case ASTNodeKind::WhileLoopStmt: {
                auto stmt = (WhileLoop*) node;
                rewrite_scope(stmt->body);
                return;
            }
            case ASTNodeKind::DoWhileLoopStmt: {
                auto stmt = (DoWhileLoop*) node;
                rewrite_scope(stmt->body);
                return;
            }
            case ASTNodeKind::ForLoopStmt: {
                auto stmt = (ForLoop*) node;
                rewrite_scope(stmt->body);
                return;
            }
            case ASTNodeKind::ForInLoopStmt: {
                auto stmt = (ForInLoop*) node;
                rewrite_scope(stmt->body);
                return;
            }
            case ASTNodeKind::SwitchStmt: {
                auto stmt = (SwitchStatement*) node;
                stmt->expression = rewrite(stmt->expression);
                for(auto& scope : stmt->scopes) {
                    rewrite_scope(scope);
                }
                return;
            }
            case ASTNodeKind::Block: {
                auto block = (BlockScope*) node;
                std::vector<ASTNode*> rebuilt;
                rebuilt.reserve(block->nodes.size());
                for(auto child : block->nodes) {
                    std::vector<ASTNode*> inner;
                    rewrite_stmt(child, inner);
                    for(auto h : inner) rebuilt.push_back(h);
                    rebuilt.push_back(child);
                }
                block->nodes = std::move(rebuilt);
                return;
            }
            default:
                return;
        }
    }

};

} // anonymous namespace

static bool normalize_scope(ASTAllocator& allocator, Scope& scope) {
    unsigned counter = 0;
    AwaitRewriter rewriter(allocator, counter);
    rewriter.rewrite_scope(scope);
    return counter > 0;
}

bool normalize_async_body(ASTAllocator& allocator, FunctionDeclaration* decl) {
    if(decl == nullptr || !decl->is_async() || !decl->body.has_value()) {
        return false;
    }
    return normalize_scope(allocator, decl->body.value());
}

bool normalize_async_lambda(ASTAllocator& allocator, LambdaFunction* lambda) {
    if(lambda == nullptr || !lambda->isAsync()) {
        return false;
    }
    return normalize_scope(allocator, lambda->scope);
}

namespace {

/**
 * Walks the (normalized) body recording frame slots and, at each await, the
 * destructible slots live at that point (design Section 8.3/8.8). Scope
 * boundaries pop the locals introduced inside them.
 */
struct PlanVisitor : public RecursiveVisitor<PlanVisitor> {

    AsyncLoweringPlan plan;
    std::vector<int> live;

    int add_slot(chem::string_view name, BaseType* type) {
        const auto id = (int) plan.slots.size();
        AsyncFrameSlot slot;
        slot.name = name;
        slot.type = type;
        slot.destructible = type != nullptr && type->get_destructor() != nullptr;
        plan.slots.push_back(slot);
        return id;
    }

    void VisitScope(Scope* scope) {
        const auto mark = live.size();
        RecursiveVisitor<PlanVisitor>::VisitScope(scope);
        live.resize(mark);
    }

    void VisitBlockScope(BlockScope* scope) {
        const auto mark = live.size();
        RecursiveVisitor<PlanVisitor>::VisitBlockScope(scope);
        live.resize(mark);
    }

    void VisitVarInitStmt(VarInitStatement* stmt) {
        const auto id = add_slot(stmt->id_view(), stmt->known_type());
        live.push_back(id);
        RecursiveVisitor<PlanVisitor>::VisitVarInitStmt(stmt);
    }

    void VisitAwaitExpression(AwaitExpression* value) {
        AwaitSite site;
        site.resume_state = (unsigned) plan.sites.size();
        site.awaited_type = value->await_result_type ? value->await_result_type : value->getType();
        // live destructible slots, reverse creation order
        for(auto it = live.rbegin(); it != live.rend(); ++it) {
            if(plan.slots[*it].destructible) {
                site.live_drops.push_back((unsigned) *it);
            }
        }
        plan.sites.push_back(site);
        RecursiveVisitor<PlanVisitor>::VisitAwaitExpression(value);
    }

    // nested async units have their own plan
    void VisitLambdaFunction(LambdaFunction*) { }

    void VisitFunctionDecl(FunctionDeclaration*) { }

};

} // anonymous namespace

AsyncLoweringPlan build_async_plan(FunctionDeclaration* decl) {
    AsyncLoweringPlan plan;
    if(decl == nullptr || !decl->body.has_value()) {
        return plan;
    }
    PlanVisitor visitor;

    // parameters live across the whole body, so they are slots 0..n-1
    for(auto param : decl->params) {
        const auto id = visitor.add_slot(param->name, param->type);
        visitor.live.push_back(id);
    }

    visitor.visit_it(decl->body.value());
    plan = std::move(visitor.plan);
    plan.slot_count = (unsigned) plan.slots.size();
    plan.needs_frame = !plan.sites.empty();
    plan.frame_align = 16;
    if(decl->returnType) {
        plan.result_has_destructor = decl->returnType->get_destructor() != nullptr;
    }
    return plan;
}
