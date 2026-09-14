---
name: Type Verification
description: Comprehensive guide to the Chemical type verification pass — how types are verified after symbol resolution, before codegen.
---

# Type Verification

The type verification pass runs after symbol resolution and before code generation. It validates that all type constraints are satisfied, catches type mismatches that symbol resolution missed, and ensures the AST is type-safe for codegen.

## Architecture

### Pipeline Position

```
Parse → Symbol Resolution (SymResLinkBody) → Type Verify → Codegen
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/typeverify/TypeVerify.h` | Type verifier class — visits all AST nodes for type checking |
| `compiler/typeverify/TypeVerify.cpp` | Implementation — type checking logic for each AST node |
| `compiler/typeverify/TypeVerifyAPI.h` | API entry point — `type_verify()` function |

## Entry Point

```cpp
// In TypeVerifyAPI.h:
void type_verify(
    ImplementationsIndex& index,
    ASTDiagnoser& diagnoser,
    ASTAllocator& allocator,
    std::span<ASTNode*> nodes
);
```

This is called once per file of a module, after symbol resolution completes, in parallel across the module's files:

```cpp
// compiler/ASTProcessor.cpp — ASTProcessor::type_verify_module_parallel
// iterates module->direct_files, each task calls type_verify_file_task:
type_verify(processor->resolver->implsIndex, diagnoser,
            processor->file_allocator, file->unit.scope.body.nodes);
```

Diagnostics from this pass are printed under the `TypeCheck` phase.

## TypeVerifier Class

```cpp
class TypeVerifier : public RecursiveVisitor<TypeVerifier> {
    ImplementationsIndex& index;       // For operator / interface impl lookup
    ASTAllocator& allocator;           // Arena for temporary allocations
    ASTDiagnoser& diagnoser;           // Error reporting

    FunctionTypeBody* current_func_type = nullptr;  // for return statement checks
    bool is_unsafe = false;                          // inside unsafe { }
    bool is_no_lifetime_check = false;               // `no_lifetime_check` unsafe flag
    bool disable_index_destructible_check = false;   // IndexOperator on LHS / &raw

    // DA state (see below): locals, init_bits, scope_stack,
    //                       da_in_unsafe, da_enabled, da_addr_inner

    // Visitor methods — one per AST node type (representative subset)
    void VisitFunctionDecl(FunctionDeclaration* decl);
    void VisitVarInitStmt(VarInitStatement* stmt);
    void VisitAssignmentStmt(AssignStatement* assign);
    void VisitFunctionCall(FunctionCall* call);
    void VisitReturnStmt(ReturnStatement* stmt);
    void VisitStructValue(StructValue* val);
    void VisitArrayValue(ArrayValue* val);
    void VisitIfStmt(IfStatement* stmt);
    void VisitWhileLoopStmt(WhileLoop* loop);
    void VisitIndexOperator(IndexOperator* value);
    void VisitUnsafeBlock(UnsafeBlock* block);
    // ... etc
};
```

## What Type Verification Checks

### 1. Function Return Types

Return values are checked in `VisitReturnStmt` against `current_func_type->returnType` (the function declaration visit only verifies default parameter values and sets up state):

```cpp
void TypeVerifier::VisitReturnStmt(ReturnStatement* node) {
    RecursiveVisitor::VisitReturnStmt(node);
    if(node->value) {
        const auto func_type = current_func_type;
        if(func_type->data.signature_resolved && func_type->returnType) {
            // constructors / matching implicit constructors skip the check
            if(!func_type->returnType->satisfies(node->value, false)) {
                unsatisfied_type_err(diagnoser, node->value, func_type->returnType);
            }
        }
    } else if(func_type->returnType->kind() != BaseTypeKind::Void) {
        diagnoser.error(node) << "function expects a non void return of type '"
                              << func_type->returnType->representation() << "'";
    }
}
```

### 2. Variable Type Compatibility

Verifies that a variable's initializer satisfies the declared type (an `@implicit` constructor is allowed to convert it):

```cpp
void TypeVerifier::VisitVarInitStmt(VarInitStatement* stmt) {
    auto& type = stmt->type;
    const auto value = stmt->value;
    const auto implicit = type->implicit_constructor_for(value);
    if(implicit == nullptr && !type->satisfies(value, false)) {
        unsatisfied_type_err(diagnoser, allocator, value, type);
    }
}
```

### 3. Assignment Type Checking

Verifies that the RHS is assignable to the (mutable, assignable) LHS:

```cpp
void TypeVerifier::VisitAssignmentStmt(AssignStatement* assign) {
    const auto lhs = assign->lhs;
    const auto value = assign->value;
    const auto lhsType = lhs->getType();

    if(!is_assignable(lhs)) {
        diagnoser.error("Expression is not assignable", lhs);
    }
    if(!lhs->check_is_mutable(true)) {
        diagnoser.error("cannot assign to a non mutable value", lhs);
    }
    if(assign->assOp == Operation::Assignment) {
        if(lhsType->implicit_constructor_for(value) == nullptr && !lhsType->satisfies(value, true)) {
            unsatisfied_type_err(diagnoser, value, lhsType);
        }
    }
}
```

### 4. Expression Type Checking

Binary/unary operator validity is checked during type inference in `Expression::get_determined_type()` (`ast/values/Expression.cpp`), not inside `TypeVerifier` (which inherits the plain `RecursiveVisitor::VisitExpression`). Primitive operands are required unless the `ImplementationsIndex` has an operator overload:

```cpp
// ast/values/Expression.cpp
const auto func = implsIndex.get_expr_op_impl(coreNodes, container, expr->operation);
if (func == nullptr) {
    diagnoser.error("expected the value to have primitive type or have operator overloaded", expr->firstValue);
} else if (func->params.size() != 2) {
    diagnoser.error(expr) << "expected operator implementation function to have exactly two parameters";
}
```

### 5. Function Call Argument Types

`VisitFunctionCall` verifies call mutability, comptime arguments, where-clause constraints, variant member arguments, and regular parameters:

```cpp
void TypeVerifier::VisitFunctionCall(FunctionCall* call) {
    RecursiveVisitor<TypeVerifier>::VisitFunctionCall(call);
    verify_call_mutability(*this, call);
    verifyArguments(call, diagnoser, current_func_type, is_interpretation_mode);

    // variant member args are checked against variant_mem->values ...
    // then regular parameters:
    for(unsigned i = 0; i < call->values.size(); i++) {
        const auto param = func_type->func_param_for_arg_at(i);
        if(param) {
            auto implicit = param->type->implicit_constructor_for(call->values[i]);
            if(implicit) { /* handle implicit constructor */ }
            else if(!param->type->satisfies(call->values[i], false)) {
                unsatisfied_type_err(diagnoser, allocator, call->values[i], param->type);
            }
        }
    }
}
```

### 6. Struct Field Types

Verifies that struct literal field values match the resolved struct member types:

```cpp
void TypeVerifier::VisitStructValue(StructValue* structValue) {
    RecursiveVisitor<TypeVerifier>::VisitStructValue(structValue);
    for (auto &val : structValue->values) {
        const auto value = val.second.value;
        const auto child_node = structValue->linked_member_or_struct_of(val.first);
        if(!child_node) {
            diagnoser.error(structValue) << "unresolved child '" << val.first << "' in struct declaration";
            continue;
        }
        const auto member = structValue->direct_variable(val.first);
        if(member) {
            const auto mem_type = member->known_type();
            auto implicit = mem_type->implicit_constructor_for(value);
            if(implicit) { /* handle implicit constructor */ }
            else if(!mem_type->satisfies(value, false)) {
                unsatisfied_type_err(diagnoser, allocator, value, mem_type);
            }
        }
    }
}
```

### 7. Implicit Conversion Checking

Some implicit conversions are allowed, and the type verifier validates them:

| From | To | Allowed? | Rule |
|------|----|----------|------|
| `int` | `long` | Yes | Widening |
| `float` | `double` | Yes | Widening |
| `*int` | `*void` | Yes | Any pointer to void pointer |
| `*T` | `*mut T` | No | Const to mutable not allowed |
| `int` | `float` | No | Requires explicit `as` cast |
| `T` | `&T` | No | Requires explicit `&` |

## The `unsatisfied_type_err` Helper

```cpp
// compiler/typeverify/TypeVerify.cpp
void unsatisfied_type_err(ASTDiagnoser& diagnoser, Value* value, BaseType* type) {
    const auto val_type = value->getType();
    if(val_type) {
        diagnoser.error(value) << "value with type '" << val_type->representation()
                               << "' does not satisfy type '" << type->representation() << "'";
    } else {
        diagnoser.error(value) << "value does not satisfy type '" << type->representation() << "'";
    }
}
```

`TypeVerifyAPI.h` also declares an inline overload `unsatisfied_type_err(diagnoser, allocator, value, type)` that forwards to the above (the `allocator` argument is currently unused).

## Integration with Operator Overloads

Operator-overload lookup uses `ImplementationsIndex::get_expr_op_impl(coreNodes, container, op)` (declared in `compiler/symres/ImplementationsIndex.h`). It is invoked from `Expression::get_overloaded_func` / `Expression::get_determined_type` (`ast/values/Expression.cpp`), not from a `TypeVerifier` method:

```cpp
// ast/values/Expression.cpp
FunctionDeclaration* Expression::get_overloaded_func(const CoreNodes& coreNodes,
                                                     const ImplementationsIndex& implsIndex) {
    const auto first_canonical = firstValue->getType()->canonical();
    const auto node = first_canonical->get_linked_canonical_node(true, false);
    if(node == nullptr) return nullptr;
    const auto container = node->get_members_container();
    if(container == nullptr) return nullptr;
    return implsIndex.get_expr_op_impl(coreNodes, container, operation);
}
```

## What's NOT Checked in Type Verify

Some checks are handled in other passes:

| Check | Pass | Reason |
|-------|------|--------|
| Move semantics | SymResLinkBody | Must be checked during linking, not after |
| Access control (public/private) | SymResLinkBody | Checked during symbol lookup |
| Generic type bounds | GenericInstantiation | Checked during monomorphization |
| Unsafe block violations | TypeVerify (+ SymResLinkBody) | `is_unsafe` gates destructible deref/index checks; `da_in_unsafe` suppresses DA checks |
| Recursion limits | Codegen or Runtime | Not a type-level check |
| Lifetime/borrow checking | TypeVerify (partial) | Temporary-lifetime check in `VisitFunctionCall`; `no_lifetime_check` unsafe flag |

## Definite-Assignment Analysis (merged into TypeVerifier)

The definite-assignment (DA) analysis was previously a separate pass (`DefiniteAssignment.cpp`)
but is now fully merged into `TypeVerifier`. It runs as part of the same AST visit, eliminating
a redundant full traversal.

### Data Structures

DA uses flat vectors for tracking (not hash sets):

```cpp
// In TypeVerifier class:
std::vector<VarInitStatement*> locals;    // Every local variable in current function
std::vector<bool> init_bits;             // Parallel: init_bits[i] == true iff locals[i] is initialized
std::vector<std::vector<VarInitStatement*>> scope_stack;  // Per-block stack for scope cleanup
bool da_in_unsafe = false;               // inside unsafe { } — suppresses DA checks
bool da_enabled = false;                 // disabled at file scope, enabled inside functions
bool da_addr_inner = false;              // next identifier is the inner of &raw/& — not a read
```

- `da_push_scope()` — push an empty frame onto `scope_stack`
- `da_pop_scope()` — erase the frame's variables from `locals`/`init_bits`, then pop the frame (linear, not O(1))
- `da_add_local(v)` — append `v` to `locals` + `init_bits(false)` and record it in the current frame
- `da_is_initialized(v)` — linear scan of `locals`, return `init_bits[index]`
- `da_root_local_var(v)` — resolve an `Identifier`/`AccessChain` to the tracked local `VarInitStatement`
- `da_type_has_destructor(v)` — true if the variable's canonical type has a destructor
- `da_report_uninit(v, action, loc)` — emit `use of uninitialized variable '...' before it is initialized (...)`
- `da_intersect(a, b, out)` — `out[i] = a[i] && (i < b.size() ? b[i] : false)`

### Rules

- A **full assignment** `x = value` (whole variable) is treated as first initialization.
- **Reading** an uninitialized variable whose type has a destructor → error.
- **Taking its address** `&raw mut x` or `&mut x` → NOT flagged (legitimate C interop).
- **Writing a member** `x.field = ...` on uninitialized destructor type → marks variable as initialized.
- Branches: variable is "definitely initialized" only if every path assigns it.

### Nested Function Isolation

When visiting a nested `FunctionDeclaration`, DA state is saved and cleared:
```cpp
auto prev_locals = std::move(locals);
auto prev_init_bits = std::move(init_bits);
auto prev_scope_stack = std::move(scope_stack);
auto prev_da_in_unsafe = da_in_unsafe;
auto prev_da_enabled = da_enabled;
locals.clear(); init_bits.clear(); scope_stack.clear();
da_in_unsafe = false; da_enabled = true;
// ... visit nested function ...
locals = std::move(prev_locals);
init_bits = std::move(prev_init_bits);
scope_stack = std::move(prev_scope_stack);
da_in_unsafe = prev_da_in_unsafe;
da_enabled = prev_da_enabled;
```

## Diagnostics

Type verification diagnostics are printed under the `TypeCheck` phase. Representative messages:

```
[TypeCheck] error: value with type 'float' does not satisfy type 'int'
[TypeCheck] error: Expression is not assignable
[TypeCheck] error: expected the value to have primitive type or have operator overloaded
[TypeCheck] error: use of uninitialized variable 'x' before it is initialized (use of)
[TypeCheck] error: index operator on a destructible type is not allowed, use `&raw` to take a pointer to the element instead
```

## Performance Considerations

1. **Single pass**: Type verification is a single pass over the AST — no backtracking
2. **Limited AST mutation**: Mostly read-only, but `VisitAssignmentStmt` marks `AssignStatement::is_first_init`, and type inference (`Expression::get_determined_type`) may coerce literal value types
3. **Early exit on errors**: If symbol resolution fails, `LabBuildCompiler` returns before type verification runs
4. **Per-file, parallel**: `ASTProcessor::type_verify_module_parallel` dispatches one task per direct file of a module
