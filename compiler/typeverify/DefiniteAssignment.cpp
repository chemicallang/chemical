// Copyright (c) Chemical Language Foundation 2025.

#include "DefiniteAssignment.h"

#include "ast/base/ASTNodeKind.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/base/Value.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/AccessChain.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/ReferenceOfValue.h"
#include "ast/values/UnsafeValue.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/Assignment.h"
#include "ast/structures/Scope.h"
#include "ast/structures/BlockScope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/If.h"
#include "ast/structures/WhileLoop.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/base/BaseType.h"

#include "preprocess/visitors/RecursiveVisitor.h"

#include <algorithm>

namespace {

/// Collects every function declaration reachable from a node list, including
/// functions nested inside namespaces / structs / impls / other functions.
class DADAFunctionCollector : public RecursiveVisitor<DADAFunctionCollector> {
public:
    std::vector<FunctionDeclaration*> fns;

    void VisitFunctionDecl(FunctionDeclaration* fn) {
        fns.emplace_back(fn);
        // Recurse into the body so nested functions are also collected.
        RecursiveVisitor<DADAFunctionCollector>::VisitFunctionDecl(fn);
    }
};

} // namespace

/// Recursively walks a value expression (or statement) and reports any use of an
/// uninitialized local variable. It is a thin wrapper over RecursiveVisitor that
/// only cares about identifier reads and address/reference-of operations.
class DefiniteAssignmentChecker : public RecursiveVisitor<DefiniteAssignmentChecker> {
public:
    DefiniteAssignment* da;
    /// Local "unsafe" override. Seeded from `da->in_unsafe` (set inside an
    /// `unsafe { }` block) and raised further when descending into `unsafe(...)`.
    bool unsafe;
    /// While true, the next identifier visit belongs to the inner of an
    /// address/reference-of and must not be reported as a standalone read.
    bool addr_inner_suppress = false;

    explicit DefiniteAssignmentChecker(DefiniteAssignment* d) : da(d), unsafe(d->in_unsafe) {}

    void VisitVariableIdentifier(VariableIdentifier* id) {
        if(addr_inner_suppress) return;
        auto* v = da->root_local_var(id);
        if(v && !da->initialized.count(v) && !unsafe) {
            if(da->type_has_destructor(v)) {
                da->report_uninit(v, "use of", id->encoded_location());
            }
        }
    }

    void VisitAddrOfValue(AddrOfValue* v) {
        auto* var = da->root_local_var(v->value);
        if(var && !da->initialized.count(var) && !unsafe) {
            da->report_uninit(var, "taking address of", v->encoded_location());
        }
        bool prev = addr_inner_suppress;
        addr_inner_suppress = true;
        RecursiveVisitor<DefiniteAssignmentChecker>::VisitAddrOfValue(v);
        addr_inner_suppress = prev;
    }

    void VisitReferenceOfValue(ReferenceOfValue* v) {
        auto* var = da->root_local_var(v->value);
        if(var && !da->initialized.count(var) && !unsafe) {
            da->report_uninit(var, "taking reference of", v->encoded_location());
        }
        bool prev = addr_inner_suppress;
        addr_inner_suppress = true;
        RecursiveVisitor<DefiniteAssignmentChecker>::VisitReferenceOfValue(v);
        addr_inner_suppress = prev;
    }

    void VisitUnsafeValue(UnsafeValue* v) {
        bool prev = unsafe;
        unsafe = true;
        RecursiveVisitor<DefiniteAssignmentChecker>::VisitUnsafeValue(v);
        unsafe = prev;
    }
};

/// Collects every function declaration reachable from a node list, including
/// functions nested inside namespaces / structs / impls / other functions.
class DAFunctionCollector : public RecursiveVisitor<DAFunctionCollector> {
public:
    std::vector<FunctionDeclaration*> fns;

    void VisitFunctionDecl(FunctionDeclaration* fn) {
        fns.emplace_back(fn);
        // Recurse into the body so nested functions are also collected.
        RecursiveVisitor<DAFunctionCollector>::VisitFunctionDecl(fn);
    }
};

void DefiniteAssignment::push_scope() {
    scope_stack.emplace_back();
}

void DefiniteAssignment::pop_scope() {
    auto& frame = scope_stack.back();
    for(auto* v : frame) {
        locals.erase(v);
        initialized.erase(v);
    }
    scope_stack.pop_back();
}

VarInitStatement* DefiniteAssignment::root_local_var(Value* v) {
    if(!v) return nullptr;
    switch(v->val_kind()) {
        case ValueKind::Identifier: {
            auto* id = static_cast<VariableIdentifier*>(v);
            auto* linked = id->linked;
            if(linked && linked->kind() == ASTNodeKind::VarInitStmt) {
                auto* var = static_cast<VarInitStatement*>(linked);
                if(locals.count(var)) return var;
            }
            return nullptr;
        }
        case ValueKind::AccessChain: {
            auto* chain = static_cast<AccessChain*>(v);
            if(!chain->values.empty()) return root_local_var(chain->values[0]);
            return nullptr;
        }
        default:
            return nullptr;
    }
}

bool DefiniteAssignment::type_has_destructor(VarInitStatement* v) {
    auto* t = v->known_type();
    if(!t) return false;
    t = t->canonical();
    return t->get_destructor() != nullptr;
}

void DefiniteAssignment::report_uninit(VarInitStatement* v, const char* action, SourceLocation loc) {
    std::string msg = "use of uninitialized variable '";
    msg.append(v->located_id.data(), v->located_id.size());
    msg += "' before it is initialized (";
    msg += action;
    msg += ")";
    diagnoser.error(chem::string_view(msg.data(), msg.size()), loc);
}

void DefiniteAssignment::check_uses(Value* value) {
    if(!value) return;
    DefiniteAssignmentChecker checker(this);
    checker.visit(value);
}

void DefiniteAssignment::check_uses(ASTNode* node) {
    if(!node) return;
    DefiniteAssignmentChecker checker(this);
    checker.visit(node);
}

void DefiniteAssignment::analyze_var_init(VarInitStatement* init) {
    scope_stack.back().push_back(init);
    locals.insert(init);
    if(init->value) {
        check_uses(init->value);
        initialized.insert(init);
    }
    // `unsafe var` / `unsafe const` is deprecated but keeps its legacy behavior:
    // uninitialized use and taking its address are permitted. Treat it as already
    // initialized so the definite-assignment checks stay silent.
    if(init->attrs.is_unsafe) {
        initialized.insert(init);
    }
    // No initializer -> variable stays uninitialized until a full assignment.
}

void DefiniteAssignment::analyze_assignment(AssignStatement* assign) {
    // RHS is always evaluated as a normal use.
    check_uses(assign->value);

    Value* lhs = assign->lhs;

    const bool is_full_assignment =
        assign->assOp == Operation::Assignment &&
        (lhs->val_kind() == ValueKind::Identifier ||
         (lhs->val_kind() == ValueKind::AccessChain &&
          static_cast<AccessChain*>(lhs)->values.size() == 1));

    if(is_full_assignment) {
        VarInitStatement* root = root_local_var(lhs);
        if(root && !initialized.count(root)) {
            // First initialization of a previously uninitialized local: the code
            // generators / interpreter must skip destroying the garbage value.
            assign->is_first_init = true;
            initialized.insert(root);
        }
        return;
    }

    // Member / index write, or a compound assignment (`+=`, `++`, ...). The root
    // variable is being accessed (read for compound, written-through for member).
    VarInitStatement* root = root_local_var(lhs);
    if(root && !initialized.count(root) && !in_unsafe) {
        if(type_has_destructor(root)) {
            report_uninit(root, "access of field/index of", lhs->encoded_location());
        }
        // Non-destructor types (e.g. arrays) may be written through safely.
    }
}

void DefiniteAssignment::analyze_nodes(std::vector<ASTNode*>& nodes) {
    push_scope();
    bool unreachable = false;
    for(auto* node : nodes) {
        if(unreachable) break;
        analyze_node(node);
        const auto k = node->kind();
        if(k == ASTNodeKind::ReturnStmt || k == ASTNodeKind::ThrowStmt ||
           k == ASTNodeKind::BreakStmt || k == ASTNodeKind::ContinueStmt) {
            unreachable = true;
        }
    }
    pop_scope();
}

void DefiniteAssignment::analyze_scope(Scope& scope) {
    analyze_nodes(scope.nodes);
}

void DefiniteAssignment::analyze_if(IfStatement* s) {
    check_uses(s->condition);
    auto before = initialized;

    initialized = before;
    analyze_scope(s->ifBody);
    auto result = initialized;

    for(auto& elif : s->elseIfs) {
        check_uses(elif.first);
        initialized = before;
        analyze_scope(elif.second);
        result = intersect(result, initialized);
    }

    if(s->elseBody.has_value()) {
        initialized = before;
        analyze_scope(s->elseBody.value());
        result = intersect(result, initialized);
    } else {
        // No else branch: the variable is only definitely initialized if it was
        // already initialized before the `if`.
        result = intersect(result, before);
    }

    initialized = result;
}

void DefiniteAssignment::analyze_while(WhileLoop* s) {
    check_uses(s->condition);
    auto before = initialized;
    // The loop body may run zero times, so its effects never propagate out.
    initialized = before;
    analyze_scope(s->body);
    initialized = before;
}

void DefiniteAssignment::analyze_do_while(DoWhileLoop* s) {
    auto before = initialized;
    initialized = before;
    analyze_scope(s->body);
    // Body runs at least once, so its effects are visible after the loop.
    check_uses(s->condition);
}

void DefiniteAssignment::analyze_for(ForLoop* s) {
    push_scope();
    if(s->initializer) analyze_node(s->initializer);
    auto before = initialized;
    if(s->conditionExpr) check_uses(s->conditionExpr);
    // Body / incrementer may run zero times.
    initialized = before;
    analyze_scope(s->body);
    initialized = before;
    if(s->incrementerExpr) analyze_node(s->incrementerExpr);
    pop_scope();
}

void DefiniteAssignment::analyze_switch(SwitchStatement* s) {
    check_uses(s->expression);
    auto before = initialized;
    auto result = before;
    for(auto& caseScope : s->scopes) {
        initialized = before;
        analyze_scope(caseScope);
        result = intersect(result, initialized);
    }
    if(!s->has_default_case()) {
        // Without a default case the switch may not enter any case.
        result = intersect(result, before);
    }
    initialized = result;
}

void DefiniteAssignment::analyze_node(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::VarInitStmt:
            analyze_var_init(static_cast<VarInitStatement*>(node));
            break;
        case ASTNodeKind::AssignmentStmt:
            analyze_assignment(static_cast<AssignStatement*>(node));
            break;
        case ASTNodeKind::Scope:
            analyze_scope(*static_cast<Scope*>(node));
            break;
        case ASTNodeKind::Block:
            analyze_nodes(static_cast<BlockScope*>(node)->nodes);
            break;
        case ASTNodeKind::IfStmt:
            analyze_if(static_cast<IfStatement*>(node));
            break;
        case ASTNodeKind::WhileLoopStmt:
            analyze_while(static_cast<WhileLoop*>(node));
            break;
        case ASTNodeKind::DoWhileLoopStmt:
            analyze_do_while(static_cast<DoWhileLoop*>(node));
            break;
        case ASTNodeKind::ForLoopStmt:
            analyze_for(static_cast<ForLoop*>(node));
            break;
        case ASTNodeKind::SwitchStmt:
            analyze_switch(static_cast<SwitchStatement*>(node));
            break;
        case ASTNodeKind::UnsafeBlock: {
            bool prev = in_unsafe;
            in_unsafe = true;
            analyze_scope(static_cast<UnsafeBlock*>(node)->scope);
            in_unsafe = prev;
            break;
        }
        case ASTNodeKind::FunctionDecl:
        case ASTNodeKind::GenericFuncDecl:
            // Nested functions are analyzed independently by the collector.
            break;
        default:
            check_uses(node);
            break;
    }
}

void DefiniteAssignment::analyze_function(FunctionDeclaration* fn) {
    locals.clear();
    initialized.clear();
    in_unsafe = false;
    scope_stack.clear();
    if(!fn->body.has_value()) return;
    analyze_scope(fn->body.value());
}

std::unordered_set<VarInitStatement*> DefiniteAssignment::intersect(
    const std::unordered_set<VarInitStatement*>& a,
    const std::unordered_set<VarInitStatement*>& b) {
    std::unordered_set<VarInitStatement*> out;
    out.reserve(std::min(a.size(), b.size()));
    for(auto* v : a) {
        if(b.count(v)) out.insert(v);
    }
    return out;
}

void definite_assignment_check(
    ImplementationsIndex& index,
    ASTDiagnoser& diagnoser,
    ASTAllocator& allocator,
    std::span<ASTNode*> nodes) {
    (void)index;
    (void)allocator;
    DAFunctionCollector collector;
    for(auto* n : nodes) collector.visit(n);
    DefiniteAssignment da(diagnoser);
    for(auto* fn : collector.fns) {
        da.analyze_function(fn);
    }
}
