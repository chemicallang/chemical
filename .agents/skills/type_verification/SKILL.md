---
name: Type Verification
description: Comprehensive guide to the Chemical type verification pass — how types are verified after symbol resolution, before codegen.
---

# Type Verification

The type verification pass runs after symbol resolution and before code generation. It validates that all type constraints are satisfied, catches type mismatches that symbol resolution missed, and ensures the AST is type-safe for codegen.

The pass is also the home of **definite-assignment analysis** (the old `DefiniteAssignment.cpp` pass was merged into `TypeVerifier` — commit `d019e8153`, "definite assignment moved to type checker, to avoid separate pass"). It enforces the `unsafe` marker rules for uninitialized variables described in AGENTS.md.

---

## The Pass in Context

### Exact pipeline position

Per module (`compiler/lab/LabBuildCompiler.cpp`):

```
Import Resolution
  → TopLevelDeclSymDeclare
  → TopLevelLinkSignature
  → generate_automatic_functions_for_module
  → GenericInstantiationPass
  → sym_res_after_signature
  → sym_res_link_body_generic_decls_pass
  → sym_res_link_body_pass
  → type_verify_module_parallel      ← THIS PASS
  → codegen (LLVM / C translation) / interpret
```

`LabBuildCompiler.cpp:819-831` calls it immediately after `processor.sym_res_module(...)` succeeds and returns `1` (skipping codegen) if it returns `false`:

```cpp
const auto sym_res_status = processor.sym_res_module(mod, pool);
if(sym_res_status != 0) { return sym_res_status; }
if(!processor.type_verify_module_parallel(pool, mod)) { return 1; }
```

The same call appears at `LabBuildCompiler.cpp:1134`, `:2981` (chemical lab module), `:4886`, and from the CBI transformer path (`compiler/cbi/bindings/TransformerContextCBI.cpp:39`).

### Inputs / outputs

Inputs: `ImplementationsIndex&` (`processor->resolver->implsIndex`, read-only impl lookups), `ASTAllocator&` (`processor->file_allocator`; threaded through, currently unused), per-file `ASTDiagnoser`, and `std::span<ASTNode*>` = `file->unit.scope.body.nodes`.

Outputs: `[TypeCheck]` diagnostics and one AST mutation downstream codegen depends on — `AssignStatement::is_first_init` is set when an assignment is a variable's first initialization (`ast/statements/Assignment.h:28`); both backends read it (`compiler/backend/LLVM.cpp:2782,2790`; `preprocess/2c/2cASTVisitor.cpp:1945`) to skip destroying the old (garbage) value. Returns a single success `bool` per module.

### Relationship to symres and codegen

- Symres **resolves** symbols and links types; TypeVerify **validates** the linked result strictly after body linking, re-reading `ImplementationsIndex`/`MembersContainer` metadata (e.g. `has_destructor()`, `InterfaceBits::COPY_BIT`).
- Checks that feel type-like but live in symres: move semantics (`SymResLinkBody`), access control (symbol lookup), constructor-vs-`@direct_init` init errors (`LinkSignature.cpp:501` / `SymResLinkBody.cpp:3107`), and operator-overload rewriting (`a + b` → `FunctionCall`; the "primitive type or operator overloaded" error comes from `Expression.cpp:137,167`).
- Codegen assumes a type-safe AST and is only reached when this pass succeeds. TypeVerify *verifies* an `@implicit` constructor is available but never inserts the call; the only flag it sets for codegen is `is_first_init`.

---

## Architecture

### Pipeline Position

```
Parse → Symbol Resolution (SymResLinkBody) → Type Verify → Codegen
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/typeverify/TypeVerify.h` | `TypeVerifier` class — visits all AST nodes for type checking, plus DA state/helpers |
| `compiler/typeverify/TypeVerify.cpp` | Implementation — type checking logic for each AST node (1915 lines) |
| `compiler/typeverify/TypeVerifyAPI.h` | API entry point — `type_verify()` and the `unsatisfied_type_err` declaration/forwarder |
| `compiler/ASTProcessor.cpp` | `type_verify_file_task` / `type_verify_module_parallel` — per-file parallel driver |
| `ast/base/BaseType.cpp` | `BaseType::satisfies` / `BaseType::implicit_constructor_for` — the satisfaction rules |
| `ast/types/*.cpp` | Per-type `satisfies` overrides (IntN, Pointer, Reference, Linked, Array, Double, Float, Bool, …) |
| `ast/values/Expression.cpp` | `get_determined_type` / `get_overloaded_func` — operator/promotion type inference |
| `compiler/ASTDiagnoser.cpp` | `ASTDiagnoser::unsatisfied_type_error` — symres-side twin of `unsatisfied_type_err` |

### Entry Point

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

Diagnostics from this pass are printed under the `TypeCheck` phase (`ASTProcessor.cpp:796`).

## TypeVerifier Class

```cpp
class TypeVerifier : public RecursiveVisitor<TypeVerifier> {
    ImplementationsIndex& index;       // For operator / interface impl lookup
    ASTAllocator& allocator;           // Arena for temporary allocations
    ASTDiagnoser& diagnoser;           // Error reporting

    FunctionTypeBody* current_func_type = nullptr;   // for return statement checks
    bool is_generic_public_context = false;          // inside a public generic decl
    bool is_public_comptime_context = false;         // inside a public comptime func
    bool is_no_lifetime_check = false;               // `no_lifetime_check` unsafe flag
    bool is_unsafe = false;                          // inside unsafe { }
    bool disable_index_destructible_check = false;   // IndexOperator on LHS / &raw
    bool is_interpretation_mode = false;             // toggled by is_interpretation() contract

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

> The `is_generic_public_context` / `is_public_comptime_context` flags exist so the
> verifier can reject calls/type references that leak non-public or non-retained
> declarations out of a public generic/comptime API surface.

---

## Visitor Architecture

This section enumerates every meaningful override and what it validates. The
`TypeVerifier` derives from `RecursiveVisitor<TypeVerifier>`, so the default
behaviour is a depth-first traversal; each override below *adds* checks.

### Declaration / container visitors

| Method | Checks |
|--------|--------|
| `VisitStructDecl` / `VisitUnionDecl` / `VisitVariantDecl` | Recurses, then `verify_container_inherited` on the container's `inherited` list: first inherited type must be a struct; every later inherited type must be an interface or an **empty** struct (`sizeof == 0`), otherwise errors. |
| `VisitInterfaceDecl` | Recurses, then requires every inherited type to be an interface (`interfaces can only inherit interfaces`). |
| `VisitImplDecl` | Recurses, then if the impl has an interface type, `verify_interface_implementation` checks (a) all inherited interfaces are implemented, and (b) every interface function without a default body has an implementation in the impl. Compiler interfaces (`extern` + `static`) are skipped. |
| `VisitFunctionDecl` | Validates default parameter values (`param->defValue`) against their declared types; sets `current_func_type`; toggles `is_public_comptime_context`; **resets DA state** for the new function (see DA section). Nested functions are fully isolated. |
| `VisitLambdaFunction` | Temporarily sets `current_func_type` to the lambda so return checks inside the lambda work. |
| `VisitGenericFuncDecl` / `VisitGenericTypeDecl` / `VisitGenericStructDecl` / `VisitGenericUnionDecl` / `VisitGenericInterfaceDecl` / `VisitGenericVariantDecl` / `VisitGenericImplDecl` | Sets `is_generic_public_context = (master_impl->specifier() == Public)`, visits the master impl, restores the flag. |
| `VisitLinkedType` | In a public generic context, rejects `LinkedType`s referring to non-public generic declarations (`using a non-public type in a public generic declaration is not allowed, please use public / protected`). **Known limitation:** linked types don't currently store a location, so the attached location is imprecise. |
| `VisitDeleteStmt` | `delete`/destruct target must be a pointer or reference (`destruct cannot be called on a value that isn't a pointer or reference`). |
| `VisitRuntimeValue` | Recurses into `value->underlying` (used by `%runtime_value(...)`). |

### Statement visitors

| Method | Checks |
|--------|--------|
| `VisitVarInitStmt` | Registers the local in DA; visits declared type + initializer; marks initialized if an initializer exists; verifies `type.satisfies(value)` unless an `@implicit` constructor applies; rejects `void`-typed variables; rejects top-level variables/constants whose type has a destructor unless `@never_destructed`. Early-returns if `!stmt->attrs.signature_resolved`. |
| `VisitAssignmentStmt` | Visits LHS as a write target (suppresses destructible-index and DA read checks), visits RHS; performs DA first-init/member-write tracking; `is_assignable(lhs)`; operator-overload shortcut for compound ops; `check_is_mutable(true)`; rejects assigning *to* a reference (must dereference explicitly); `verify_mutation` for struct members; type-satisfaction of RHS vs LHS (`implicit_constructor_for` aware); pointer `+=`/`-=` accepts an `IntN` RHS. |
| `VisitReturnStmt` | Verifies the return value satisfies `current_func_type->returnType` (unless the function is a constructor or an applicable implicit constructor of the same container); a missing value in a non-void function errors. |
| `VisitIfStmt` | Handles `comptime if` contract toggling (e.g. `intrinsics::is_interpretation()` flips `is_interpretation_mode` per branch); DA branch intersection. |
| `VisitWhileLoopStmt` / `VisitDoWhileLoopStmt` / `VisitForLoopStmt` | DA loop handling: an initialization inside a loop body does **not** escape the loop (state resets to pre-loop); `for` pushes/pops a DA scope. |
| `VisitSwitchStmt` | DA intersection across all case scopes plus the pre-switch state if there is no default. |
| `VisitScope` / `VisitBlockScope` | Push/pop DA scope so locals die at block exit. |

### Value / expression visitors

| Method | Checks |
|--------|--------|
| `VisitFunctionCall` | The busiest visitor — see "Function Call Argument Types" below. Also verifies call mutability, comptime arguments, retention in public generic/comptime context, temporary-lifetime escape, `where`-clause constraints, variant-member args, and regular params. |
| `VisitStructValue` | Every struct-literal field must resolve to a child (`unresolved child '…' in struct declaration`) and satisfy the member's type (implicit-constructor aware). |
| `VisitArrayValue` | Element type may not be `void`; every element must satisfy the element type (implicit-constructor aware); struct-element case is checked separately. |
| `VisitFunctionCall` (variant ctor path) | When the callee is a `VariantMember`, each arg is checked against the variant payload type. |
| `VisitIndexOperator` | `check_destructible_index`: indexing a destructible container is rejected unless wrapped in `&raw`/`&`/on the LHS (`... use \`&raw\` to take a pointer to the element instead`); array indexing is exempt because it yields an lvalue/reference. Non-`Copy` generic type params are rejected. Gated by `disable_index_destructible_check`. |
| `VisitDereferenceValue` | `check_destructible_deref`: de-referencing a pointer/reference to a destructible struct is rejected; non-`Copy` generic type params are rejected. |
| `VisitAddrOfValue` (`&raw`) | Suppresses inner destructible-index/DA checks; rejects `&raw` of a non-struct r-value; rejects taking the address of a `comptime` variable or a non-top-level `const`. |
| `VisitReferenceOfValue` (`&`) | Same suppression; rejects `&`-of-r-value; rejects reference of a `comptime` variable, mutable reference of a `const`, or reference of a primitive `const`. |
| `VisitPlacementNewValue` | `verify_placement_new`: the placement pointer must be a pointer (after peeling type aliases) and the value must satisfy the pointee type. |
| `VisitIncDecValue` | `verify_mutation` — `++`/`--` on a struct member requires a mutable `self`. |
| `VisitPatternMatchExpr` | Disables destructible-deref checking for the matched expression and else-expression (pattern extraction does not copy). |
| `VisitUnsafeValue` | DA: sets `da_addr_inner` over the whole `unsafe(...)` expression so inner reads are not DA-checked. |
| `VisitVariableIdentifier` | DA: reports a read of an uninitialized destructible local (`use of uninitialized variable '…' before it is initialized (use of)`), unless `da_addr_inner`/`da_in_unsafe`. |
| `VisitUnsafeBlock` | Sets `is_unsafe` + `da_in_unsafe`; a named flag (`unsafe(no_lifetime_check) { … }`) toggles a specific `TypeVerifier` flag via `get_flag` (currently only `no_lifetime_check`), or warns `couldn't find flag`. |

### Checks performed outside `TypeVerifier`

| Check | Where |
|-------|-------|
| Binary/unary operator validity + result-type promotion | `Expression::get_determined_type` (`ast/values/Expression.cpp:121`) |
| Operator-overload lookup | `Expression::get_overloaded_func` / `ImplementationsIndex::get_expr_op_impl` |
| Move semantics | `SymResLinkBody` (symres) |
| Constructor-vs-`@direct_init` struct init | `LinkSignature.cpp` / `SymResLinkBody.cpp` |

---

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

The real implementation (`TypeVerify.cpp:1595-1628`) additionally skips the check when the current function is a constructor (`func->is_constructor_fn()`) or when the returned value has an implicit constructor belonging to a different parent container.

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

`VisitVarInitStmt` also rejects `void`-typed variables and top-level destructible variables (without `@never_destructed`).

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

`is_assignable` (`TypeVerify.cpp:1438`) whitelists lvalue-capable value kinds (`Identifier`, `AccessChain`, `IndexOperator`, `DereferenceValue`, `FunctionCall` returning a reference, …) and rejects literals, math expressions, addresses, struct/array literals, lambdas, etc.

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

`get_determined_type` is also where integer-literal promotion is decided: if one operand is an untyped integer literal, it is coerced to the other operand's type when it fits. `Expression::get_determined_type` is invoked from `Expression::setType` (`ast/values/Expression.h:109`).

### 5. Function Call Argument Types

`VisitFunctionCall` (`TypeVerify.cpp:962-1166`) verifies call mutability, comptime arguments, where-clause constraints, variant member arguments, retention/public-generic rules, lifetime escape, and regular parameters:

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

Additional checks inside `VisitFunctionCall`:

- **Call mutability** (`verify_call_mutability`, `TypeVerify.cpp:560`): a mutable-`self` method requires a mutable receiver chain; implicit-`self` calls require a mutable `self` param. Errors: `call requires a mutable implicit self argument...`, `cannot call function without an implicit self arg which is not present`.
- **Comptime argument verification** (`verifyArgumentsAreComptime` / `getNonComptimeValue`): a `comptime` function requires arguments known at compile time; a runtime value triggers `comptime function expects argument that is known at compile time`. Skipped in interpretation mode (`is_interpretation_mode`).
- **`where`-clause constraints** (`verify_where_clause`, `TypeVerify.cpp:877`): for a generic call/struct method, resolves the concrete type argument and checks `Copy` (no destructor) or an impl exists. Errors: `type '…' does not satisfy where clause constraint '… : …'`.
- **Retention / access in public generic/comptime context** (`TypeVerify.cpp:973-1082`): in `is_generic_public_context`, rejects calling non-public / non-retained declarations; in `is_public_comptime_context`, rejects calling non-retained functions. See the `@retained` annotation in the [annotations skill](../annotations/SKILL.md).
- **Temporary lifetime escape** (`TypeVerify.cpp:1088-1117`): a method call on a temporary destructible struct that returns a struct with a lifetime dependency errors `function call on a temporary that is destroyed at expression end returns a struct with a lifetime dependency, please store the temporary in a variable`. Disabled by `is_no_lifetime_check`.

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

Some implicit conversions are allowed, and the type verifier validates them through `BaseType::satisfies` and the per-type overrides. See the full table in **Implicit Conversion Rules** below.

### 8. Inheritance / Interface Implementation

- `verify_container_inherited` (`TypeVerify.cpp:137`): the first inherited type must be a struct; subsequent entries must be interfaces or empty structs.
- `VisitInterfaceDecl`: interfaces may only inherit interfaces.
- `verify_interface_implementation` (`TypeVerify.cpp:1204`): every interface member without a default body must be implemented by the `impl`; inherited interfaces must also be implemented.

### 9. Destructible Dereference / Index / Copy Constraints

- Non-`Copy` generic type parameters cannot be de-referenced or indexed unless `is_unsafe` (`de-referencing a reference/pointer to generic type parameter that is not \`Copy\` is not allowed`).
- De-referencing a pointer/reference to a destructible struct is rejected (`de-referencing a reference/pointer to destructible struct is not allowed`).
- Indexing a destructible container is rejected (`index operator on a destructible type is not allowed, use \`&raw\` to take a pointer to the element instead`). Array indexing is exempt.
- Both checks are bypassed when the operator is wrapped in `&raw`/`&` or appears on the assignment LHS.

### 10. Address-of / Reference Rules

`&raw` (`VisitAddrOfValue`) and `&` (`VisitReferenceOfValue`) reject:

- r-values (`cannot apply operator '&raw' to r-value` / `cannot apply operator '&' to r-value`);
- `comptime` variables (`taking address/reference of a comptime variable is not allowed`);
- non-top-level `const` addresses (`taking address of a constant is not allowed`);
- mutable references to `const` (`taking mutable reference of a constant is not allowed`);
- references to primitive `const` (`taking reference of a primitive constant is not allowed`).

### 11. Mutation Requires Mutable `self`

`verify_mutation` (`TypeVerify.cpp:183`): assigning to a struct member from within a method requires a mutable `self` reference (methods with no `self` are exempt if the function is a constructor). Error: `mutating a struct member requires a mutable self reference`.

### 12. `void` and Top-Level Destructible Rules

- `VisitVarInitStmt`: a variable may not have type `void` (`variable with name '…' type can't be of type void`).
- A top-level variable/constant whose type has a destructor must use `@never_destructed` (`top level variables or constants must be non-destructible, or must use @never_destructed annotation`).
- `VisitArrayValue`: `array element type cannot be void`.

### 13. Async Rules

- `VisitAwaitExpression` (`TypeVerify.cpp:1734`): `await` is legal only when
  `current_func_type->isAsync()` — otherwise
  `` `await` can only be used inside an `async` function, `async` closure, or `async` block ``.
- **Return checks unwrap the await type.** `async func f() : T` has the
  symres-wrapped static return type `FutureHandle<T>`, but its body returns `T`.
  `VisitReturnStmt` compares against `FunctionDeclaration::inner_return_type()`
  (the unwrapped `T`), not `returnType`.
- `@extern async` is rejected: `an @extern function cannot be async; declare it
  with an explicit FutureHandle<T> ABI` (`TypeVerify.cpp:1658`).
- An async function may not be a destructor (`cannot be a destructor`).
- A `void` async function's inner type is `core::async::Unit`, so the
  non-void-return check treats it as void-like.

---

## Implicit Conversion Rules

These are the rules actually implemented by `satisfies` overrides. `assignment` is the boolean passed by callers (`true` for assignment LHS, `false` elsewhere).

| From | To | Allowed? | Rule / source |
|------|----|----------|---------------|
| Integer literal (`IntN` value, untyped) | any `IntN` | Yes | `IntNType::satisfies(Value*)` short-circuits for integer literals (`IntNType.cpp:159-168`). |
| `IntN` | `IntN` | Yes, iff **same signedness** (any width) | `IntNType::satisfies(IntNType*)` returns true when both signed or both unsigned (`IntNType.h:109`). So `int → long` and `long → int` both pass; `uint → int` fails. |
| `int`/`uint` family | `enum` with matching underlying integer | Yes | `IntNType::satisfies` canonicalizes enums (`IntNType.cpp:142`). |
| `bool` | `bool` (or `&bool`) | Yes | `BoolType::satisfies` (`IntNType.cpp:21`). |
| `float` | `float` | Yes | `FloatType::satisfies` only accepts `Float` (`FloatType.h:21`). |
| `double` | `double` | Yes | `DoubleType::satisfies` only accepts `Double` (`DoubleType.h:21`). |
| `float` | `double` | **No** | `DoubleType` only accepts `Double`; no float/double widening is implemented. |
| `int` | `float` / `double` | **No** | Requires explicit `as` cast. |
| `nullptr` | any pointer | Yes | `PointerType::satisfies` returns true for `NullPtr` (`PointerType.cpp:33`). |
| `*T` | `*void` | Yes | `PointerType::satisfies` with `type_kind == Void` (`PointerType.cpp:56`). |
| `*T` | `*const T` | Yes | Mutable → immutable pointee is fine. |
| `*const T` | `*mut T` | **No** | `if(!pointer->is_mutable && is_mutable) return false;` (`PointerType.cpp:53`). |
| `[N]T` | `*T` | Yes (decay) | `PointerType::satisfies` handles `Array` (`PointerType.cpp:44`). |
| string literal | `*char` / `*uchar` (immutable) | Yes | `PointerType::satisfies` string overload (`PointerType.cpp:38`). |
| function | `*void` | Yes | `PointerType.cpp:48`. |
| `[N]char` / string | `[N]char` | Yes | `ArrayType::satisfies` (`Types.cpp:28`). |
| `T` | `&T` | Only for immutable refs and r-values where inner satisfies (no assignment) | `ReferenceType::satisfies` (`PointerType.cpp:73-109`). Mutable ref binding requires an l-value/reference. |
| any struct field value | declared member type | via `satisfies(value, false)` | `VisitStructValue`. |
| value with `@implicit` ctor | target type | via `implicit_constructor_for` | `MembersContainer::implicit_constructor_func` (`MembersContainer.cpp:635`) and `BaseType::implicit_constructor_for` (`BaseType.cpp:441`). |

> **Gotcha:** `IntNType::satisfies(IntNType*)` deliberately allows *both directions* and only checks signedness. The comment in `IntNType.h:105` says "types that are larger in bits or smaller can satisfy each other as long as they are same signed". Do not assume width-based widening.

> **Gotcha:** `float`/`double` are **not** interchangeable. `0.5` is a `double` literal and does not satisfy a `float` parameter; use `0.5f` (see AGENTS.md "Chemical Library Development Gotchas").

---

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

Symres has a parallel method `ASTDiagnoser::unsatisfied_type_error(Value*, BaseType*)` (`compiler/ASTDiagnoser.cpp:32`) that produces the same message shape; it is used by `LinkSignature.cpp:823`.

---

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

`TypeVerifier` also uses `ImplementationsIndex::get_impl(interface, for)` for `where`-clause constraints and `verify_interface_implementation`. See the [symres skill](../symres/SKILL.md) and [generics skill](../generics/SKILL.md) for the impl-index details.

---

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

- A **full assignment** `x = value` (whole variable, or an `AccessChain` of length 1) is treated as first initialization (`AssignStatement::is_first_init = true`).
- **Reading** an uninitialized variable whose type has a destructor → error. Types without a destructor (primitives etc.) are silently allowed even before initialization (they cannot be double-destroyed).
- **Taking its address** `&raw mut x` or `&mut x` → NOT flagged (legitimate C interop). Implemented by setting `da_addr_inner = true` while visiting `AddrOfValue` / `ReferenceOfValue`.
- **Writing a member** `x.field = ...` / `x[i] = ...` on uninitialized destructor type → marks the *root* variable as initialized.
- Branches: variable is "definitely initialized" only if every path assigns it.
- **`unsafe(expr)`** suppresses the read check for the inner expression (`VisitUnsafeValue` sets `da_addr_inner`).
- **Inside `unsafe { }`** all DA read checks are suppressed (`da_in_unsafe`).

### Where the analysis is wired in

| Location | DA action |
|----------|-----------|
| `VisitFunctionDecl` | Resets `locals`/`init_bits`/`scope_stack`, sets `da_enabled = true`, restores on exit (nested-function isolation). |
| `VisitVarInitStmt` | `da_add_local`; mark initialized when an initializer exists. |
| `VisitAssignmentStmt` | First-init detection + member/index write initialization. |
| `VisitVariableIdentifier` | The actual uninitialized-read check. |
| `VisitIfStmt` / `VisitSwitchStmt` | Branch intersection via `da_intersect`. |
| `VisitWhileLoopStmt` / `VisitDoWhileLoopStmt` / `VisitForLoopStmt` | Loop bodies do not propagate initializations after the loop. |
| `VisitScope` / `VisitBlockScope` | Scope push/pop so block locals die at block exit. |
| `VisitUnsafeBlock` / `VisitUnsafeValue` | Suppress checks. |
| `VisitAddrOfValue` / `VisitReferenceOfValue` | Mark the inner as an address, not a read. |

### Branch semantics in detail

`VisitIfStmt` (`TypeVerify.cpp:1381-1413`) visits the condition, snapshots `before = init_bits`, then for each branch resets `init_bits = before`, visits the branch, and intersects its end state into `result` with `da_intersect`. If there is no `else`, the fall-through path (`before`) is intersected as an implicit branch. The practical consequence: a variable is initialized *after* an `if` only if it was initialized on the taken branch **and** on every other branch (or was already initialized before). `switch` follows the same pattern; a missing `default` forces intersection with the pre-switch state. Loops reset to the pre-loop state after the body, so an initialization inside a loop never escapes it.

### Full assignment vs. member write

An assignment is a *full assignment* when the op is `=` and the LHS is an `Identifier` or a length-1 `AccessChain`. For a tracked root local that is not yet initialized, this sets `AssignStatement::is_first_init = true` (codegen then skips destroying the old garbage value) and marks the root initialized. Any *member/index* write (`x.field = ...`, `x[i] = ...`) also marks the root variable initialized, but does not set `is_first_init`. `is_first_init` is the only DA output consumed by codegen (`compiler/backend/LLVM.cpp:2782,2790`, `preprocess/2c/2cASTVisitor.cpp:1945`).

### Exemptions

- **Global / `@extern` declarations**: DA is disabled at file scope (`da_enabled` is false until entering a function), and only locals registered via `da_add_local` are tracked. Globals are never in `locals`.
- **Struct member declarations**: `da_root_local_var` only resolves to `VarInitStatement`; struct fields (a different AST node kind) are never tracked.
- **Variables inside `unsafe { }`**: DA read checks are suppressed while `da_in_unsafe` is true.
- **Address/reference of a value**: `&raw mut x` / `&mut x` are exempt (they are address computations, not reads).
- **`unsafe(expr)`**: suppresses the read check for the wrapped expression.
- **Non-destructible types**: only variables whose canonical type has a destructor are reported.

### AGENTS.md library-development rules (verified against source)

The rules in AGENTS.md's "Chemical Library Development Gotchas → Uninitialized Variables and the `unsafe` Marker" match the implementation:

- Declare uninitialized variables as plain `var x : Type` / `const x : Type`; the old `unsafe var` syntax is a parse error.
- Reading an uninitialized variable whose type has a destructor → `use of uninitialized variable 'x' before it is initialized (use of)` (`TypeVerify.cpp:1690-1698`, `da_report_uninit` at `:108`).
- Taking its address `&raw mut x` / `&mut x` is **not** flagged (`VisitAddrOfValue` / `VisitReferenceOfValue` set `da_addr_inner`; `da_report_uninit` is never reached from there).
- Writing a member/index `x.field = ...` / `x[i] = ...` marks the variable initialized (the `else` branch of `VisitAssignmentStmt`).
- `unsafe(expr)` is a compile-time-only marker that asserts intentional access of uninitialized memory; `unsafe { }` blocks suppress checks for their whole body.
- Library migration guidance: do **not** add `unsafe` to a declaration; either initialize the variable or wrap the specific access with `unsafe(...)`. Bulk-edit scripts must skip `var` inside struct/variant/enum bodies because struct fields are exempt.

### Nested Function Isolation

When visiting a nested `FunctionDeclaration`, DA state is saved and cleared, giving every function (including nested ones) an independent DA universe:

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

---

## Categorized Error Reference

All messages are emitted under the `TypeCheck` phase tag. `file:line` columns point at the emission site in `compiler/typeverify/TypeVerify.cpp` unless noted.

### Type satisfaction / mismatch

| Message | Trigger | Fix |
|---------|---------|-----|
| `value with type 'X' does not satisfy type 'Y'` | `satisfies()` returned false for an initializer, assignment, argument, field, array element, return, or default param (`TypeVerify.cpp:237`, `unsatisfied_type_err`) | Convert with an explicit `as` cast, change the declared type, or add an `@implicit` constructor. |
| `value does not satisfy type 'Y'` | Same, but the value has no computable type (`:239`) | Give the expression a concrete type. |
| `value with type 'double' does not satisfy type 'float'` | `float`/`double` are not interchangeable (`DoubleType.h:21`, `FloatType.h:21`) | Use a `float` literal (`0.5f`) or cast `as float`. |
| `value with type '…' does not satisfy type '…'` (int signedness) | `IntNType::satisfies` requires matching signedness (`IntNType.h:109`) | Cast to the matching signedness. |
| `type 'X' does not satisfy where clause constraint 'T : Iface'` | `verify_where_clause` (`:939`, `:953`) | Implement the interface / remove the destructor for a `Copy` bound, or change the type argument. |
| `value doesn't satisfy the overloaded operator parameter` | `Expression::get_determined_type` (`Expression.cpp:147`) | Match the operator impl's parameter type. |
| `value does not satisfy the pointer value type` | `verify_placement_new` (`:281`) | Make the placement value match the pointee. |
| `expected pointer value to be of pointer type` | `verify_placement_new` (`:296`) | Placement target must be a pointer (aliases peeled). |

### Assignability / mutability

| Message | Trigger | Fix |
|---------|---------|-----|
| `Expression is not assignable` | `is_assignable(lhs)` returned false (`:1539`) | Assign to an lvalue (identifier, field, index, deref). |
| `cannot assign to a non mutable value` | `!lhs->check_is_mutable(true)` (`:1557`) | Declare the variable/member `mut`/`var`, or take a mutable reference. |
| `assignment to reference is forbidden, please use dereference operator explicitly` | Assigning to a reference-typed LHS directly (`:1562`) | Use `*ref = value`. |
| `mutating a struct member requires a mutable self reference` | `verify_mutation` (`:198`) | Change the method's `self` to `&mut self`. |
| `call requires a mutable implicit self argument, however current self argument is not mutable` | `verify_call_mutability` (`:624`, `:634`) | Make `self` mutable or the receiver chain mutable. |
| `cannot call function without an implicit self arg which is not present` | Calling a `self` method with no receiver (`:632`) | Provide a receiver. |

### Destructible dereference / index / Copy

| Message | Trigger | Fix |
|---------|---------|-----|
| `de-referencing a reference/pointer to destructible struct is not allowed` | `check_destructible_deref` (`:325`) | Wrap in `&raw`/`&`, or make the type non-destructible. |
| `de-referencing a reference/pointer to generic type parameter that is not \`Copy\` is not allowed` | `check_destructible_deref` (`:320`) | Add a `Copy` bound or an `unsafe` context. |
| `index operator on a destructible type is not allowed, use \`&raw\` to take a pointer to the element instead` | `check_destructible_index` (`:354`) | Use `&raw` to get an element pointer. |
| `index operator on a generic type parameter that is not \`Copy\` is not allowed, use \`&raw\` to take a pointer to the element instead` | `check_destructible_index` (`:349`) | Add a `Copy` bound / use `&raw`. |

### Address-of / reference

| Message | Trigger | Fix |
|---------|---------|-----|
| `cannot apply operator '&raw' to r-value` / `cannot apply operator '&' to r-value` | `:382`, `:419` | Store the temporary in a variable first. |
| `taking address of a comptime variable is not allowed` / `taking reference of a comptime variable is not allowed` | `:390`, `:427` | Use a runtime `var`. |
| `taking address of a constant is not allowed` | `&raw` of a non-top-level `const` (`:393`) | Make it a `var` or remove `const`. |
| `taking mutable reference of a constant is not allowed` | `&mut` of a `const` (`:431`) | Make it a `var`. |
| `taking reference of a primitive constant is not allowed` | `&` of a primitive `const` (`:435`) | Make it a `var`. |

### Comptime / retention / generics

| Message | Trigger | Fix |
|---------|---------|-----|
| `comptime function expects argument that is known at compile time` | `verifyComptimeArgument` (`:800`) | Pass a compile-time-known value. |
| `calling a non-retained function in a public generic declaration is not allowed, please use @retained annotation` | `VisitFunctionCall` (`:982`, NonRetainedDeclCallError) | Add `@retained` to the callee. |
| `calling a non-retained function in a public comptime declaration is not allowed, please use @retained annotation` | `VisitFunctionCall` (`:1037`) | Add `@retained`. |
| `non-retained decl is being called in a public generic context, please use @retained annotation` | companion `diagnoser.info` (`:983`) | — |
| `calling a non-public function in a public generic declaration is not allowed, please use public/protected` | NonRetainedDeclCallError family (`:466`) | Make the callee `public`/`protected`. |
| `using a non-public type in a public generic declaration is not allowed, please use public / protected` | `VisitLinkedType` (`:1798`) | Make the type public/protected. |

### Interfaces / inheritance

| Message | Trigger | Fix |
|---------|---------|-----|
| `the type in inheritance list must be a struct` | `verify_container_inherited` (`:145`, `:166`) | Only structs/interfaces in an inheritance list. |
| `struct type is not empty (contains variables) being inherited (not in the first position) in inheritance list` | `verify_container_inherited` (`:157`, `:163`) | Move the struct first or make it empty. |
| `interfaces can only inherit interfaces` | `VisitInterfaceDecl` (`:229`) | Inherit an interface, not a struct. |
| `no implementation of interface 'X' could be found for 'Y'` | `verify_interface_implementation` (`:1226`) | Add the missing inherited impl. |
| `type does not implement interface member 'f'` | `verify_interface_implementation` (`:1244`, `:1258`) | Implement `f` in the `impl`. |
| `function hasn't been implemented in an impl below` | companion `diagnoser.warn` (`:1245`, `:1259`) | — |

### Misc / declarations

| Message | Trigger | Fix |
|---------|---------|-----|
| `variable with name 'x' type can't be of type void` | `VisitVarInitStmt` (`:1311`) | Use a concrete type. |
| `top level variables or constants must be non-destructible, or must use @never_destructed annotation` | `VisitVarInitStmt` (`:1316`) | Add `@never_destructed` or use a non-destructible type. |
| `array element type cannot be void` | `VisitArrayValue` (`:248`) | Use a concrete element type. |
| `unresolved child 'f' in struct declaration` | `VisitStructValue` (`:1175`) | Fix the field name / resolution failure. |
| `destruct cannot be called on a value that isn't a pointer or reference` | `VisitDeleteStmt` (`:1633`) | Pass a pointer/reference to `delete`. |
| `function expects a non void return of type 'T'` | `VisitReturnStmt` (`:1624`) | Return a value of type `T`. |
| `function call on a temporary that is destroyed at expression end returns a struct with a lifetime dependency, please store the temporary in a variable` | `VisitFunctionCall` (`:1111`) | Bind the temporary to a variable. |
| `couldn't find flag` (warning) | `VisitUnsafeBlock` with an unknown named flag (`:1895`) | Use a supported flag name (currently `no_lifetime_check`). |
| `expected the value to have primitive type or have operator overloaded` | `Expression::get_determined_type` (`Expression.cpp:137`, `:167`) | Add an operator overload or use primitives. |
| `expected operator implementation function to have exactly two parameters` | `Expression.cpp:142`, `:172` | Fix the overload signature. |

---

## What's NOT Checked in Type Verify

Some checks are handled in other passes:

| Check | Pass | Reason |
|-------|------|--------|
| Move semantics | SymResLinkBody | Must be checked during linking, not after |
| Access control (public/private) | SymResLinkBody | Checked during symbol lookup |
| Generic monomorphization | GenericInstantiation | Concrete types are created during instantiation; **call-site `where`-clause constraints are additionally verified by TypeVerify** (`verify_where_clause`) |
| Unsafe block violations | TypeVerify (+ SymResLinkBody) | `is_unsafe` gates destructible deref/index checks; `da_in_unsafe` suppresses DA checks |
| Recursion limits | Codegen or Runtime | Not a type-level check |
| Lifetime/borrow checking | TypeVerify (partial) | Temporary-lifetime check in `VisitFunctionCall`; `no_lifetime_check` unsafe flag |
| Operator-overload resolution itself | SymResLinkBody + `Expression::get_determined_type` | TypeVerify only re-reads the impl index |
| Constructor-vs-`@direct_init` init syntax | LinkSignature / SymResLinkBody | Emitted during signature/body linking |

---

## Diagnostics

Type verification diagnostics are printed under the `TypeCheck` phase. Representative messages:

```
[TypeCheck] error: value with type 'float' does not satisfy type 'int'
[TypeCheck] error: Expression is not assignable
[TypeCheck] error: expected the value to have primitive type or have operator overloaded
[TypeCheck] error: use of uninitialized variable 'x' before it is initialized (use of)
[TypeCheck] error: index operator on a destructible type is not allowed, use `&raw` to take a pointer to the element instead
```

The `TypeCheck` tag is passed to `Diagnoser::print_diagnostics` in `ASTProcessor.cpp:796`. Diagnostics are collected into a per-file `ASTDiagnoser` and moved out as `std::vector<Diag>`. See the [diagnostics skill](../diagnostics/SKILL.md) for the `Diag`/`ASTDiagnoser` API.

---

## Invocation, Parallelization and Error Merging

Each file is verified by its own task with its own `TypeVerifier`/`ASTDiagnoser` (`compiler/ASTProcessor.cpp:754-802`):

- `type_verify_file_task` constructs a local `ASTDiagnoser(processor->loc_man)`, calls `type_verify(index, diagnoser, allocator, file->unit.scope.body.nodes)`, and returns `TypeVerifyFileResult { has_errors, diagnostics }` (diagnostics are *moved* out).
- `ASTProcessor::type_verify_module_parallel` pushes one task per `module->direct_files` entry into the `ctpl` pool, then drains futures in submission order, ORs `has_errors` into `success`, and prints each non-empty diagnostic vector under `print_mutex` with the `"TypeCheck"` tag.
- The shared `ImplementationsIndex` is queried read-only (its internal `std::shared_mutex` allows concurrent readers). There is no shared verifier or DA state — DA is per file, hence per function.
- Unlike symres phases, this driver does **not** consult `ASTProcessorOptions::stop_on_file_error`; every file runs and all diagnostics print. `LabBuildCompiler` then decides whether to continue to codegen.

For interpretation jobs `TypeVerify` still runs; `is_interpretation_mode` (toggled by the `intrinsics::is_interpretation()` contract inside `comptime if`) merely causes `verifyArguments` to skip comptime-argument checks. Interpretation does not bypass type verification.

---

## Debugging a Type Error

1. **Read the phase tag.** `[TypeCheck]` errors come from this pass; `[SymRes:...]` errors come from symres and usually need fixing first.
2. **Find the emission site** in `compiler/typeverify/TypeVerify.cpp` using the Categorized Error Reference above.
3. **Isolate and reproduce** the failing code in `lang/compiled/temp.ch` (see AGENTS.md "Debugging: Isolating a Single Test Case"), then compile with `cmake-build-debug/TCCCompiler "lang/compiled/temp.ch" -o "lang/compiled/temp.c" -v -bm-modules`.
4. **Check the actual type.** `BaseType::representation()` produces the names in the message. For a `double`/`float` mismatch remember `0.5` is `double` — use `0.5f`; for integer mismatches check *signedness*, not width (`IntNType::satisfies`).
5. **Check `@implicit`.** A missing implicit constructor surfaces as an ordinary `does not satisfy` error.
6. **For DA errors**, decide between adding an initializer or wrapping the access in `unsafe(...)`.
7. **Add debug output** to the failing `TypeVerifier` method and rebuild (`./scripts/build.sh --tcc` or `--llvm`); `--no-build` reuses a stale binary after C++ edits. For crashes use `gdb -batch -ex run -ex bt`.
8. **Test both backends/modes**: `./scripts/test.sh --tcc` and `./scripts/test.sh --tcc --interpret`.

---

## Adding a New Check

1. **Declare/define the visitor override** in `TypeVerify.h` / `TypeVerify.cpp` and call `RecursiveVisitor::VisitXxx(node)` first (unless you deliberately control traversal for DA/flag toggling, as `VisitIfStmt`/loops do).
2. **Emit diagnostics** with `diagnoser.error(node_or_value) << "message"`, `diagnoser.warn(...)`, or `unsatisfied_type_err(diagnoser, value, type)`; attach to the most specific child value for a useful location.
3. **Reuse helpers**: `da_*`, `verify_mutation`/`check_chain_mutability`, `verify_where_clause`, `verify_container_inherited`, `verify_interface_implementation`.
4. **Save/restore stateful flags** (`is_unsafe`, `is_generic_public_context`, `is_public_comptime_context`, `current_func_type`, DA state) like the existing overrides, and keep the pass read-only apart from `is_first_init` and literal coercion.
5. **Test** with a `@test` function, a `lang/tests/negative/` case, or `lang/compiled/temp.ch`; rebuild before validating.
6. A new `unsafe` named flag must be mapped in `get_flag` (`TypeVerify.cpp:1880`).

---

## Cross-links

[symres](../symres/SKILL.md) (preceding pass) · [generics](../generics/SKILL.md) (monomorphization) · [diagnostics](../diagnostics/SKILL.md) (`ASTDiagnoser`/`Diag`) · [annotations](../annotations/SKILL.md) (`@retained`, `@implicit`, `@never_destructed`, `@direct_init`, `@make`) · [interpreter](../interpreter/SKILL.md) (interpretation/comptime) · [testing](../testing/SKILL.md) · [language syntax](../chemical_source/SKILL.md) · [LLVM backend](../llvm_backend/SKILL.md) / [C codegen](../c_codegen/SKILL.md) (consumers of `is_first_init`).

---

## Performance Considerations

1. **Single pass**: Type verification is a single pass over the AST — no backtracking
2. **Limited AST mutation**: Mostly read-only, but `VisitAssignmentStmt` marks `AssignStatement::is_first_init`, and type inference (`Expression::get_determined_type`) may coerce literal value types
3. **Early exit on errors**: If symbol resolution fails, `LabBuildCompiler` returns before type verification runs
4. **Per-file, parallel**: `ASTProcessor::type_verify_module_parallel` dispatches one task per direct file of a module
5. **Linear DA bookkeeping**: DA uses flat `std::vector` scans (`da_*`), so reporting is O(locals) per identifier, and `da_pop_scope` is linear in the frame size — acceptable because function-local variable counts are small
6. **No shared mutable verifier state**: Every file task owns its own `TypeVerifier` and `ASTDiagnoser`; only the `ImplementationsIndex` is shared and is read-only here
7. **`chem::string_view` messages**: error construction is done through `diagnoser.error(...) << ...` without intermediate `std::string` except where dynamic text is needed
