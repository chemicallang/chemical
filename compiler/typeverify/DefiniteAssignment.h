// Copyright (c) Chemical Language Foundation 2025.
//
// Definite-assignment analysis.
//
// This pass runs during the type-verify stage. It tracks which local variables
// are *definitely initialized* at each program point and reports an error when
// a variable is used before it is initialized.
//
// Design goals (per compiler requirements):
//   * Allow `var x : Type` without an initializer or `unsafe` keyword.
//   * Error on any *use* of an uninitialized variable whose type has a
//     destructor (reading it would copy/destruct garbage memory).
//   * A full assignment `x = value` to an uninitialized variable is its first
//     initialization: it does NOT destroy the previous (garbage) value. We mark
//     the assignment with `AssignStatement::is_first_init` so the interpreter and
//     code generators skip the destructor call.
//   * Member / index writes like `s[i] = 5` on an uninitialized variable are
//     allowed when the variable's type has no destructor (e.g. arrays). For
//     destructible types they are still an error (`x.field = string()`).
//   * Taking a pointer/reference to an uninitialized variable
//     (`&raw mut x` / `&mut x`) always requires `unsafe(...)`, regardless of the
//     type, because the resulting pointer escapes uninitialized memory.

#pragma once

#include "ast/base/ASTNode.h"
#include <unordered_set>
#include <vector>
#include <span>

namespace chem { class string_view; }

class ASTDiagnoser;
class VarInitStatement;
class Value;
class Scope;
class FunctionDeclaration;
class ASTAllocator;
class ImplementationsIndex;

class DefiniteAssignment {
    ASTDiagnoser& diagnoser;

    /// Every local variable declaration we encountered inside the function. Only
    /// variables in this set are subject to the uninitialized-use checks (globals,
    /// parameters and `self` are never checked).
    std::unordered_set<VarInitStatement*> locals;

    /// Variables that are *definitely initialized* on the path that reaches the
    /// current statement.
    std::unordered_set<VarInitStatement*> initialized;

    /// True while we are inside an `unsafe { }` block or `unsafe(...)` expression,
    /// where taking addresses / reading uninitialized variables is permitted.
    bool in_unsafe = false;

    /// Per-block stack of declarations, so variables leave scope cleanly.
    std::vector<std::vector<VarInitStatement*>> scope_stack;

    void analyze_scope(Scope& scope);
    void analyze_nodes(std::vector<ASTNode*>& nodes);
    void analyze_node(ASTNode* node);
    void analyze_var_init(VarInitStatement* init);
    void analyze_assignment(AssignStatement* assign);
    void analyze_if(IfStatement* s);
    void analyze_while(WhileLoop* s);
    void analyze_do_while(DoWhileLoop* s);
    void analyze_for(ForLoop* s);
    void analyze_switch(SwitchStatement* s);

    /// Recursively inspects a value expression (or statement) and reports uses of
    /// uninitialized local variables (reads and address-of).
    void check_uses(Value* value);
    void check_uses(ASTNode* node);

    /// Resolves the *root local variable* of an lvalue (identifier or access chain).
    /// Returns nullptr when the value is not rooted in a function-local variable.
    VarInitStatement* root_local_var(Value* v);

    bool type_has_destructor(VarInitStatement* v);

    void push_scope();
    void pop_scope();

    void report_uninit(VarInitStatement* v, const char* action, SourceLocation loc);

    static std::unordered_set<VarInitStatement*> intersect(
        const std::unordered_set<VarInitStatement*>& a,
        const std::unordered_set<VarInitStatement*>& b
    );

    friend class DefiniteAssignmentChecker;

public:
    explicit DefiniteAssignment(ASTDiagnoser& diagnoser) : diagnoser(diagnoser) {}

    void analyze_function(FunctionDeclaration* fn);
};

/// Collects all `FunctionDeclaration`s reachable from `nodes` (including nested
/// ones inside namespaces / structs / impls) and runs the analysis on each.
void definite_assignment_check(
    ImplementationsIndex& index,
    ASTDiagnoser& diagnoser,
    ASTAllocator& allocator,
    std::span<ASTNode*> nodes
);
