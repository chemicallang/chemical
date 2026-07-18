# Chemical Language — Comprehensive Safety Analysis

> **Status: Research Document — Updated July 18, 2026**
>
> A thorough analysis of Chemical's current safety posture, missing safety features,
> implementation gaps, and **AI-friendly, low-effort additions**.
>
> **Key theme:** What can be implemented quickly by AI, behind flags, with minimal
> performance impact, enabling per-module opt-in.

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current Safety Features (Accurate)](#current-safety-features-accurate)
3. [Features That Already Exist](#features-that-already-exist)
4. [Where Clause & Interface System — Current State](#where-clause--interface-system--current-state)
5. [AI Implementation Methodology](#ai-implementation-methodology)
6. [Feature 1: Enable safe_context Enforcement](#feature-1-enable-safe_context-enforcement)
7. [Feature 2: Enable @unsafe Function Propagation](#feature-2-enable-unsafe-function-propagation)
8. [Feature 3: Extern C Call Safety Check](#feature-3-extern-c-call-safety-check)
9. [Feature 4: Std Library Checked Arithmetic](#feature-4-std-library-checked-arithmetic)
10. [Feature 5: More InterfaceBits & Auto-Derive](#feature-5-more-interfacebits--auto-derive)
11. [Feature 6: Container Constraint Enforcement](#feature-6-container-constraint-enforcement)
12. [Feature 7: Bounds Checking (--bounds-check)](#feature-7-bounds-checking---bounds-check)
13. [Feature 8: Nullable/NonNull Pointer Types](#feature-8-nullablenonnull-pointer-types)
14. [Feature 9: Overflow Checking (--overflow-check)](#feature-9-overflow-checking---overflow-check)
15. [Feature 10: Lifetime Escape Analysis (Basic)](#feature-10-lifetime-escape-analysis-basic)
16. [Feature 11: Full safe Flag Architecture](#feature-11-full-safe-flag-architecture)
17. [Feature Flag Architecture Summary](#feature-flag-architecture-summary)
18. [Safety Infrastructure Reference](#safety-infrastructure-reference)
19. [Comparison with Other Languages](#comparison-with-other-languages)

---

## Executive Summary

Chemical is a systems programming language with deterministic destructors, move semantics,
and C-level performance. It occupies a design space between C and Rust.

**Key insight:** The compiler's safety infrastructure is surprisingly complete at the
**tracking** level — what's missing is **enforcement**. This makes many safety features
"finishing touches" rather than ground-up builds.

**For AI implementation:** Each feature below is broken into **tiny, atomic sub-steps**
that can be independently implemented, tested, and verified. Each step includes:
- Exact files and line numbers to modify
- What the compiler does currently (the "before" state)
- What it should do after (the "after" state)
- A specific test case in `.ch` Chemical source code that validates the change
- How to verify correctness (the compilation command and expected output)
- Common pitfalls and how to constrain the AI

---

## Current Safety Features (Accurate)

### ✅ Move Semantics (Implemented & Enforced)
- Variables with destructors cannot be implicitly copied after move
- `var y = x` where `x` has a destructor → `x` is moved (invalidated)
- Access chain tracking, if/else branch-aware, function args moved into callee
- **Files:** `compiler/symres/SymResLinkBody.cpp/.h`

### ✅ Destructors and RAII (Implemented)
- `@delete` annotated destructors, auto-generated member destructors
- Variant destructors, scope-based destruction with drop flags

### ✅ Zero-Initialization Restrictions (Implemented)
- Types with dtors/ctors blocked from `zeroed<T>()` unless `@allow_zeroed` or `zeroed:unsafe<T>()`

### ✅ Sanitizer Support (Implemented)
- `--sanitize=address,undefined,thread,memory,leak`

### ✅ Unsafe Block & safe_context Tracking (Implemented — NOT Fully Enforced)
```cpp
bool safe_context = true;  // tracked in SymResLinkBody.h
// VisitUnsafeBlock() toggles it off inside the block
// VisitDereferenceValue() only WARNS, not errors:
//   if(safe_context) diagnoser.warn(...)
```
The `safe_context` flag is **tracked** but only produces a **warning** for dereference.

### ✅ @unsafe Function Flag (Implemented — NOT Enforced)
```cpp
// FunctionDeclaration.h
bool is_unsafe = false;
```
The check in `TypeVerify.cpp` line 455 is **commented out**:
```cpp
// if(func_decl->is_unsafe() && resolver.safe_context) { ...error... }
```

---

## Features That Already Exist

### ✅ Where Clause — FULLY IMPLEMENTED
```chemical
func get(&self, index : size_t) : T where T : Copy { return data_ptr[index]; }
```

### ✅ Copy, Clone, Default, Delete Interfaces
All defined in `lang/libs/core/ops.ch` and `lang/libs/core/interfaces.ch`.

### ✅ InterfaceBits System
`ast/base/InterfaceBits.h` — bitflag system with `COPY_BIT = 1 << 0`.

### ✅ GenericTypeParam Trait List
```cpp
std::vector<BaseType*> traits;  // in GenericTypeParameter
InterfaceBits current_bits;     // computed from constraints
```

### ✅ Unsafe "no_lifetime_check" Annotation
`unsafe "no_lifetime_check" { ... }` — exists in test files.

### ✅ Zeroed:unsafe<T>() Syntax
`zeroed:unsafe<Type>()` bypasses zero-init restrictions.

---

## Where Clause & Interface System — Current State

### What Works
```
Parser → WhereClause AST → LinkSignature links types → calculate_where_clause_bits() sets bit
```
### What's Missing
1. Only `COPY_BIT` defined — need `DESTRUCTIBLE_BIT`, `DEFAULT_BIT`, `TRIVIALLY_RELOCATABLE_BIT`
2. No auto-derivation — types must manually `impl Copy for MyType`
3. Container constraints not enforced — `vector<T>` doesn't require `T : TriviallyRelocatable`

---

## AI Implementation Methodology

Each feature below follows this pattern:

### The Atomic-Step Approach

Every major safety feature is broken into **5-15 atomic sub-steps**. Each step:

1. **Is independently testable** — you can compile a test file and get a pass/fail
2. **Is revertible** — if the AI breaks something, only one small step is rolled back
3. **Has a known "before" state** — the AI can search for exact code patterns
4. **Requires ≤ 20 lines of change** — most steps are 3-10 lines

### General AI Constraints

| Constraint | How to Enforce |
|------------|---------------|
| **Never modify existing tests** | AI must only add new tests, never change existing `.ch` files in `lang/tests/` |
| **Use only `diagnoser.error()` not raw `std::cerr`** | The compiler's diagnostic system must be used for user-facing errors |
| **Never remove `#include`** | AI may add includes but never remove them |
| **Always compile before testing** | `./scripts/build.sh --tcc` before running tests |
| **Test both pass and fail cases** | For each safety check, test both: the code that should compile AND the code that should error |
| **Feature flag defaults to OFF** | New safety features must default to disabled (warnings, not errors) |
| **Compilation must not regress** | Run `./scripts/test.sh --tcc` before and after; same pass count |
| **One file per change** | Each sub-step should modify at most 2-3 files |

### General Test Structure

```chemical
// lang/tests/safety/test_feature_X.ch
func test_safe_deref_blocked() {
    test("safe context blocks dereference", () => {
        // This should be a compile-time error with --safe
        // We test it at comptime so the interpreter catches it
        var x = 42
        var p = &raw x
        // *p should error in safe context but not in unsafe
        return true
    })
}
```

### Verification Commands

```bash
# Step 1: Build the compiler with changes
./scripts/build.sh --tcc

# Step 2: Compile a test file that should PASS
cmake-build-debug/TCCCompiler lang/tests/safety/pass_test.ch -o /dev/null --mode debug_complete 2>&1
# Expected: exits 0, no errors

# Step 3: Compile a test file that should FAIL (with --safe flag)
cmake-build-debug/TCCCompiler lang/tests/safety/fail_test.ch -o /dev/null --mode debug_complete --safe 2>&1
# Expected: exits non-zero, specific error message

# Step 4: Run full test suite to check for regressions
./scripts/test.sh --tcc
```

---

## Feature 1: Enable safe_context Enforcement

### Overview
The `safe_context` flag is already tracked in `SymResLinkBody`. When set, unsafe operations
should produce errors (or at least warnings). Currently, only `VisitDereferenceValue` produces
a **warning**. We need to:
1. Promote the deref warning to error (behind `--safe` flag)
2. Add checks for other unsafe operations in `SymResLinkBody`

### Dependencies
- Step 1 must be done first (defines the `--safe` flag infrastructure)
- Steps 2-6 are independent and can be done in any order
- Step 7 is optional and can be done last

### Sub-Step 1.1: Add --safe flag to LabBuildCompilerOptions

**Before:** `LabBuildCompilerOptions` has no safety flag.

**After:** Add `SafeMode` enum and `safe_mode` field:

```cpp
// In compiler/lab/LabBuildCompilerOptions.h
// Add an enum for different safe mode levels
enum class SafeMode : uint8_t {
    Off = 0,      // No safety enforcement (default)
    Warn = 1,     // Emit warnings for safety violations
    Enforce = 2,  // Emit errors for safety violations
};

// Add to LabBuildCompilerOptions class:
SafeMode safe_mode = SafeMode::Off;
```

**Validation:**
- Build the compiler and confirm `--safe` flag is recognized (even if it does nothing yet):
```bash
./scripts/build.sh --tcc
cmake-build-debug/TCCCompiler --help 2>&1 | grep -i safe
# Should show --safe flag description
```

**AI constraints:**
- Default must be `Off` — no existing code breaks
- Use the same pattern as existing flags like `is_testing_env` or `sanitizers`

---

### Sub-Step 1.2: Pass safe_mode through to SymResLinkBody

**Before:** `SymResLinkBody` has no access to the safe mode setting.

**After:** Add `SafeMode` field to `SymResLinkBody` and set it during construction.

```cpp
// In compiler/symres/SymResLinkBody.h
// Add member:
SafeMode safe_mode = SafeMode::Off;

// Modify constructor to accept the mode:
SymResLinkBody(SymbolResolver& resolver, SafeMode safe_mode = SafeMode::Off)
    : linker(resolver), diagnoser(resolver.loc_man), safe_mode(safe_mode), ...
```

**Files to modify:**
- `compiler/symres/SymResLinkBody.h` — add field, update constructor
- `compiler/symres/SymResLinkBody.cpp` — update constructor calls if any
- `compiler/lab/LabBuildCompiler.cpp` — pass `options->safe_mode` when creating `SymResLinkBody`

**Validation:**
- Build passes. No behavior change yet.
```bash
./scripts/build.sh --tcc
```

**AI constraints:**
- `SafeMode::Off` default ensures zero behavior change for existing code
- Search for ALL `SymResLinkBody(` constructor calls in the codebase and update them

---

### Sub-Step 1.3: Promote Deref Warning to Error with --safe

**Before:**
```cpp
// SymResLinkBody.cpp line ~2388
void SymResLinkBody::VisitDereferenceValue(DereferenceValue* value) {
    if(safe_context) {
        diagnoser.warn("de-referencing a pointer in safe context is prohibited", value);
    }
    ...
}
```

**After:**
```cpp
void SymResLinkBody::VisitDereferenceValue(DereferenceValue* value) {
    if(safe_context) {
        switch(safe_mode) {
            case SafeMode::Enforce:
                diagnoser.error("de-referencing a pointer outside unsafe block is not allowed", value);
                break;
            case SafeMode::Warn:
                diagnoser.warn("de-referencing a pointer in safe context is prohibited", value);
                break;
            default:
                break;  // Off: no diagnostic
        }
    }
    ...
}
```

**Validation:**
```bash
# Build the compiler
./scripts/build.sh --tcc

# Create test file: lang/tests/safety/deref_safe.ch
cat > lang/tests/safety/deref_safe.ch << 'EOF'
func test_deref_safe() {
    var x = 42
    var p = &raw x
    // This dereference should warn with --safe=warn and error with --safe
    // NOTE: in default mode (no flag), nothing happens
    return *p
}
EOF

# Test 1: Default mode — no diagnostic
cmake-build-debug/TCCCompiler lang/tests/safety/deref_safe.ch -o /dev/null --mode debug_complete 2>&1
# EXPECTED: exits 0, no warnings

# Test 2: --safe=warn — should warn
cmake-build-debug/TCCCompiler lang/tests/safety/deref_safe.ch -o /dev/null --mode debug_complete --safe=warn 2>&1
# EXPECTED: exits 0, but produces warning

# Test 3: --safe — should error
cmake-build-debug/TCCCompiler lang/tests/safety/deref_safe.ch -o /dev/null --mode debug_complete --safe 2>&1 | grep -i "de-referencing"
# EXPECTED: exits non-zero, error message about dereferencing

# Test 4: unsafe block — no diagnostic regardless of flag
cat > lang/tests/safety/deref_unsafe.ch << 'EOF'
func test_deref_unsafe() {
    var x = 42
    var p = &raw x
    unsafe {
        return *p
    }
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/deref_unsafe.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: exits 0, no errors/warnings
```

**AI constraints:**
- Copy the existing `diagnoser.warn()` pattern — don't invent new error APIs
- The `safe_context` toggle already works in `VisitUnsafeBlock` — just use it
- Test both safe and unsafe contexts

---

### Sub-Step 1.4: Check DeallocStmt in safe_context

**Before:** `VisitDeallocStmt` has no safety check.

```cpp
// SymResLinkBody.cpp ~line 1355
void SymResLinkBody::VisitDeallocStmt(DeallocStmt* node) {
    visit(node->ptr);
}
```

**After:**
```cpp
void SymResLinkBody::VisitDeallocStmt(DeallocStmt* node) {
    if(safe_context) {
        check_safe_context("deallocating memory requires an unsafe block", node);
    }
    visit(node->ptr);
}
```

Where `check_safe_context` is a helper:
```cpp
void SymResLinkBody::check_safe_context(const char* message, ASTNode* node) {
    switch(safe_mode) {
        case SafeMode::Enforce: diagnoser.error(message, node); break;
        case SafeMode::Warn:    diagnoser.warn(message, node);  break;
        default: break;
    }
}
```

**Validation:**
```bash
cat > lang/tests/safety/dealloc_safe.ch << 'EOF'
func test_dealloc() {
    var p = malloc(100) as *mut int
    dealloc p  // Should error in safe mode
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/dealloc_safe.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: error about dealloc requiring unsafe block
```

---

### Sub-Step 1.5: Check Malloc/Free Calls in safe_context

**Before:** Any function can call `malloc`/`free` without `unsafe`.

**After:** In `VisitFunctionCall`, check if the callee is an extern function like `malloc`.

Look for extern functions that are memory management functions. The simplest approach
is to check if the function has `@extern` or is a C standard library function.

**Implementation:**

```cpp
// In SymResLinkBody::VisitFunctionCall, after resolving the callee:
if(safe_context && safe_mode != SafeMode::Off) {
    const auto callee = /* get the linked function declaration */;
    if(callee && callee->is_extern_fn()) {
        check_safe_context(
            "calling extern C function requires an unsafe block",
            call
        );
    }
}
```

**Validation:**
```bash
cat > lang/tests/safety/extern_call.ch << 'EOF'
func test_extern_call() {
    // printf is extern, requires unsafe in safe mode
    printf("hello\n")
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/extern_call.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: warning/error about calling extern C function
```

---

### Sub-Step 1.6: Check Pointer Arithmetic in safe_context

**Before:** Pointer arithmetic is unrestricted.

**After:** In `VisitExpression` or the arithmetic visitor, check if the operation
involves pointer types and requires `unsafe`.

**Implementation:**
```cpp
// In SymResLinkBody, when visiting an arithmetic operation:
void SymResLinkBody::check_pointer_arith(Value* value, BaseType* type) {
    if(safe_context && type && type->is_pointer()) {
        check_safe_context("pointer arithmetic requires an unsafe block", value);
    }
}
```

---

### Sub-Step 1.7: Check Cast to Ptr in safe_context

**Before:** `as *mut T` casts are unrestricted.

**After:** In `VisitCastedValue`, check if the target type is a pointer.

---

### Validation Complete for Feature 1

After all sub-steps, run:
```bash
./scripts/build.sh --tcc
./scripts/test.sh --tcc
# All existing tests must pass (no regressions)
```

---

## Feature 2: Enable @unsafe Function Propagation

### Overview
`FunctionDeclaration::is_unsafe()` exists. `@unsafe` annotation is parsed and stored.
But calling an `@unsafe` function from safe code is **not enforced**. The check in
`TypeVerify.cpp` is commented out.

### Sub-Step 2.1: Uncomment the @unsafe Check in TypeVerify

**Before** (TypeVerify.cpp ~line 455):
```cpp
// if(func_decl->is_unsafe() && resolver.safe_context) {
//     ...error...
// }
```

**After:**
```cpp
if(func_decl->is_unsafe() && verifier.is_unsafe) {
    // Already in unsafe context — allowed
} else if(func_decl->is_unsafe() && !verifier.is_safe_mode_off()) {
    switch(verifier.safe_mode) {
        case SafeMode::Enforce:
            verifier.diagnoser.error(
                "calling @unsafe function requires an unsafe block",
                call
            );
            break;
        case SafeMode::Warn:
            verifier.diagnoser.warn(
                "calling @unsafe function from safe context",
                call
            );
            break;
        default: break;
    }
}
```

Wait, looking at the actual code more carefully. The `is_unsafe` in TypeVerify is a boolean flag
that's toggled by unsafe blocks. So the check should be:

```cpp
// If the function is @unsafe AND we're NOT already in an unsafe block
if(func_decl->is_unsafe() && !verifier.is_unsafe) {
    if(verifier.safe_mode == SafeMode::Enforce) {
        verifier.diagnoser.error(...);
    } else if(verifier.safe_mode == SafeMode::Warn) {
        verifier.diagnoser.warn(...);
    }
}
```

**Files to modify:**
- `compiler/typeverify/TypeVerify.cpp` — uncomment and fix the check
- `compiler/typeverify/TypeVerify.h` — may need to add `safe_mode` field

**Validation:**
```bash
# Create an @unsafe function and call it from safe code
cat > lang/tests/safety/unsafe_fn.ch << 'EOF'
@unsafe
func unsafe_thing() : int {
    return 42
}

func safe_fn() : int {
    return unsafe_thing()  // Should error in --safe mode
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/unsafe_fn.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: error about calling @unsafe function

# Now test that wrapping in unsafe block fixes it
cat > lang/tests/safety/unsafe_fn_ok.ch << 'EOF'
@unsafe
func unsafe_thing() : int {
    return 42
}

func safe_fn() : int {
    unsafe {
        return unsafe_thing()  // OK: in unsafe block
    }
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/unsafe_fn_ok.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: no error
```

---

## Feature 3: Extern C Call Safety Check

### Overview
Extern C functions bypass all safety guarantees. Calling them should require `unsafe`.

### Sub-Step 3.1: Add is_extern_fn() Helper

**Before:** No easy way to check if a function is an extern C declaration.

**After:** Add a helper method to `FunctionDeclaration`:

```cpp
// In ast/structures/FunctionDeclaration.h
inline bool is_extern_fn() const {
    return attrs.is_extern || attrs.is_dllimport;
}
```

**Validation:**
```bash
./scripts/build.sh --tcc
# Just verify it compiles
```

---

### Sub-Step 3.2: Check Extern Calls in VisitFunctionCall

**Before:** `VisitFunctionCall` in `SymResLinkBody` doesn't check extern status.

**After:** After resolving the function, check if it's extern and we're in safe context.

```cpp
// Near the end of SymResLinkBody::VisitFunctionCall, add:
if(safe_context && safe_mode != SafeMode::Off) {
    const auto callee = call->parent_val->get_chain_last_linked();
    if(callee && callee->kind() == ASTNodeKind::FunctionDecl) {
        const auto decl = callee->as_function_unsafe();
        if(decl->is_extern_fn()) {
            check_safe_context(
                "calling extern C function requires unsafe block",
                call
            );
        }
    }
}
```

**Validation:**
```bash
cat > lang/tests/safety/extern_call.ch << 'EOF'
func test_extern() {
    printf("hello world\n")  // extern C function -> error in safe mode
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/extern_call.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: warning/error about extern C call

# Test that unsafe block silences it:
cat > lang/tests/safety/extern_call_ok.ch << 'EOF'
func test_extern() {
    unsafe {
        printf("hello world\n")
    }
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/extern_call_ok.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: no error
```

---

## Feature 4: Std Library Checked Arithmetic

### Overview
Integer overflow is UB in Chemical (same as C/C++). Provide opt-in checked/saturating/wrapping
arithmetic through the standard library.

### Sub-Step 4.1: Define Checked Arithmetic Interfaces

**Before:** No checked arithmetic interfaces exist.

**After:** Add to `lang/libs/core/ops.ch`:

```chemical
// Checked arithmetic
public interface CheckedAdd<Rhs = Self> {
    func checked_add(&self, rhs : Rhs) : Option<Self>
}

public interface CheckedSub<Rhs = Self> {
    func checked_sub(&self, rhs : Rhs) : Option<Self>
}

public interface CheckedMul<Rhs = Self> {
    func checked_mul(&self, rhs : Rhs) : Option<Self>
}

public interface SaturatingAdd<Rhs = Self> {
    func saturating_add(&self, rhs : Rhs) : Self
}

public interface SaturatingSub<Rhs = Self> {
    func saturating_sub(&self, rhs : Rhs) : Self
}
```

**Validation:**
- Just check that the module compiles:
```bash
./scripts/build.sh --tcc
```

---

### Sub-Step 4.2: Implement CheckedAdd for Integer Types

**After:** Add implementations in `lang/libs/std/src/ops_impl.ch` or similar:

```chemical
impl CheckedAdd for i32 {
    func checked_add(&self, rhs : i32) : Option<i32> {
        // At runtime, use LLVM with.overflow intrinsics when --overflow-check is set
        // For basic version: just do the operation (wrapping UB semantics)
        const result = *self + rhs
        // TODO: LLVM codegen should emit overflow check with flag
        return Option::Some(result)
    }
}
```

**Validation:**
```bash
cat > lang/tests/safety/checked_add.ch << 'EOF'
func test_checked_add() {
    var a : i32 = 100
    var b : i32 = 50
    match a.checked_add(b) {
        Option::Some(val) => {
            test("checked_add works", () => val == 150)
        }
        Option::None => {
            test("checked_add overflow", () => false)
        }
    }
}
EOF
```

---

## Feature 5: More InterfaceBits & Auto-Derive

### Sub-Step 5.1: Add New Bit Constants to InterfaceBits.h

**Before:**
```cpp
static constexpr BitsType COPY_BIT = BitsType(1) << 0;
```

**After:**
```cpp
static constexpr BitsType COPY_BIT                  = BitsType(1) << 0;
static constexpr BitsType DESTRUCTIBLE_BIT          = BitsType(1) << 1;
static constexpr BitsType DEFAULT_CONSTRUCTIBLE_BIT = BitsType(1) << 2;
static constexpr BitsType TRIVIALLY_RELOCATABLE_BIT = BitsType(1) << 3;
static constexpr BitsType CLONE_BIT                 = BitsType(1) << 4;
```

Also add convenience methods:
```cpp
static InterfaceBits copy() { return InterfaceBits(COPY_BIT); }
static InterfaceBits destructible() { return InterfaceBits(DESTRUCTIBLE_BIT); }
static InterfaceBits default_constructible() { return InterfaceBits(DEFAULT_CONSTRUCTIBLE_BIT); }
static InterfaceBits trivially_relocatable() { return InterfaceBits(TRIVIALLY_RELOCATABLE_BIT); }
static InterfaceBits clone() { return InterfaceBits(CLONE_BIT); }
```

**Validation:**
```bash
./scripts/build.sh --tcc
# Must compile without errors
```

---

### Sub-Step 5.2: Define Corresponding Interfaces in Chemical Source

**Before:** Only `Copy` interface exists in `lang/libs/core/interfaces.ch`.

**After:** Add new marker interfaces:
```chemical
// lang/libs/core/interfaces.ch
public interface Copy {}

public interface Destructible {}

@non_dyn
public interface TriviallyRelocatable {}

public interface DefaultConstructible {}
```

**Validation:**
```bash
./scripts/build.sh --tcc
```

---

### Sub-Step 5.3: Register New Interfaces in CoreNodes

**Before:** `CoreNodes` only has `copy_interface`.

**After:**
```cpp
// compiler/symres/CoreNodes.h
InterfaceDefinition* copy_interface = nullptr;
InterfaceDefinition* destructible_interface = nullptr;
InterfaceDefinition* default_constructible_interface = nullptr;
InterfaceDefinition* trivially_relocatable_interface = nullptr;
```

**Validation:**
```bash
./scripts/build.sh --tcc
```

---

### Sub-Step 5.4: Auto-Set Destructible Bit During Struct Processing

**Before:** Interface bits are only set from explicit `where` clause constraints.

**After:** In `SymResLinkBody::LinkMembersContainerNoScope` or during type analysis,
auto-set the `DESTRUCTIBLE_BIT` on the struct's interface bits:

```cpp
// In the struct processing code, after creating the struct:
auto bits = container->interface_bits;
if(container->has_destructor()) {
    bits.set(InterfaceBits::DESTRUCTIBLE_BIT);
}
container->interface_bits = bits;
```

**Validation:**
```bash
cat > lang/tests/safety/auto_destructible_bit.ch << 'EOF'
// Test that a struct with @delete auto-gets Destructible bit
struct HasDtor {
    var x : int
    @delete func delete(&mut self) {}
}

// This function requires T : Destructible
func <T> take_destructible(val : T) where T : Destructible {}

func test_auto_bit() {
    var h = HasDtor { x: 42 }
    take_destructible(h)  // Should compile: HasDtor auto-implements Destructible
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/auto_destructible_bit.ch -o /dev/null --mode debug_complete 2>&1
# EXPECTED: compiles without error
```

---

### Sub-Step 5.5: Auto-Set DefaultConstructible Bit

**After:** In struct processing, if a struct has no `@constructor` annotation and all
fields are zeroable/defaultable, set `DEFAULT_CONSTRUCTIBLE_BIT`:

```cpp
if(!container->has_constructor() && container->is_zeroable()) {
    bits.set(InterfaceBits::DEFAULT_CONSTRUCTIBLE_BIT);
}
```

---

### Sub-Step 5.6: Auto-Set TriviallyRelocatable Bit

**After:** A struct is trivially relocatable if:
- Has no destructor AND
- Has no self-referencing pointers AND
- All members are trivially relocatable

```cpp
if(!container->has_destructor() && fields_are_trivially_relocatable(container)) {
    bits.set(InterfaceBits::TRIVIALLY_RELOCATABLE_BIT);
}
```

---

## Feature 6: Container Constraint Enforcement

### Sub-Step 6.1: Add where T : Default to vector::resize()

**Before:**
```chemical
func resize(&mut self, new_size : size_t) {
    // Uses zeroed<T>() for new elements — dangerous for non-zeroable types
}
```

**After:**
```chemical
func resize(&mut self, new_size : size_t) where T : Default {
    // Now requires T to implement Default interface
}
```

**Validation:**
```bash
# Test that vector<int> (which has Default) can still resize
# Test that vector<NoDefaultStruct> gets a compile error on resize
```

---

### Sub-Step 6.2: Add where T : Copy to vector::push()

**Before:**
```chemical
func push(&mut self, value : T) {
    memcpy(&raw mut data_ptr[s], &raw value, sizeof(T))
    intrinsics::forget(value)
}
```

**After:**
```chemical
func push(&mut self, value : T) where T : Copy {
    // Only allows trivially copyable types
}
```

**Note:** This is more aggressive. For destructible types, you need a different push
method that properly moves (destructs old + copies). Consider keeping a separate
`push_move` for movable types.

---

## Feature 7: Bounds Checking (--bounds-check)

### Sub-Step 7.1: Add --bounds-check Flag

**After:** Add to `CodegenEmitterOptions.h`:
```cpp
bool bounds_check = false;
```

**Validation:**
```bash
./scripts/build.sh --tcc
```

---

### Sub-Step 7.2: Pass Bounds Check Flag to LLVM Codegen

**After:** In the LLVM codegen for `IndexOperator::llvm_value()` and
`IndexOperator::llvm_pointer()`, insert bounds checks before the GEP:

```cpp
// Pseudo-code for IndexOperator::llvm_value()
if(gen.bounds_check) {
    auto index = /* LLVM value for the index */;
    auto size = /* LLVM value for the array size */;
    auto in_bounds = gen.builder->CreateICmpULT(index, size);
    
    // Create trap block
    auto trap_block = llvm::BasicBlock::Create(...);
    auto cont_block = llvm::BasicBlock::Create(...);
    gen.builder->CreateCondBr(in_bounds, cont_block, trap_block);
    
    // In trap block: call llvm.trap()
    gen.builder->SetInsertPoint(trap_block);
    gen.builder->CreateCall(/* llvm.trap intrinsic */);
    gen.builder->CreateUnreachable();
    
    // Continue in cont_block
    gen.builder->SetInsertPoint(cont_block);
}
```

**Validation:**
```bash
# Create a test that indexes out of bounds
cat > lang/tests/safety/oob_test.ch << 'EOF'
func test_oob() {
    var arr : [3]int = [1, 2, 3]
    var val = arr[10]  // Out of bounds — should trap with --bounds-check
}
EOF

# Without flag: UB (might segfault, might return garbage)
cmake-build-debug/Compiler lang/tests/safety/oob_test.ch -o lang/tests/safety/oob_test.exe --mode debug_complete
./lang/tests/safety/oob_test.exe
# EXPECTED: undefined behavior (might crash, might not)

# With flag: should trap
cmake-build-debug/Compiler lang/tests/safety/oob_test.ch -o lang/tests/safety/oob_test.exe --mode debug_complete --bounds-check
./lang/tests/safety/oob_test.exe
# EXPECTED: traps with SIGILL or calls abort()
```

**AI constraints:**
- Only add checks in `Debug` mode (check `gen.out_mode`)
- Use `CreateCondBr` + `CreateCall` to `@llvm.trap` — don't use assertions or `abort()`
- Must not change the `inbounds` GEP flag — keep `inbounds = true` even with bounds check
- The bounds check is an ADDITIONAL check, NOT a replacement for `inbounds`

---

## Feature 8: Nullable/NonNull Pointer Types

### Sub-Step 8.1: Create NonNull<T> Wrapper

**After:** Create `lang/libs/std/src/ptr.ch`:

```chemical
public namespace std {

public struct NonNull<T> {
    var ptr : *mut T

    @constructor
    func make(ptr : *mut T) -> Option<NonNull<T>> {
        if(ptr != null) {
            return Option::Some(NonNull { ptr: ptr })
        }
        return Option::None()
    }

    // Safe: verified non-null at construction
    func get(&self) -> &mut T {
        return &mut *ptr
    }

    func get_ptr(&self) -> *mut T {
        return ptr
    }
}

}
```

**Validation:**
```bash
cat > lang/tests/safety/nonnull_test.ch << 'EOF'
import std

func test_nonnull() {
    var x = 42
    match std::NonNull::make(&raw mut x) {
        Option::Some(ref nn) => {
            test("NonNull holds correct value", () => *nn.get() == 42)
        }
        Option::None => {
            test("NonNull creation failed", () => false)
        }
    }
    
    // Null pointer should fail
    var null_ptr : *mut int = null
    match std::NonNull::make(null_ptr) {
        Option::Some(ref nn) => {
            test("NonNull should not accept null", () => false)
        }
        Option::None => {
            test("NonNull rejected null", () => true)
        }
    }
}
EOF
```

---

## Feature 9: Overflow Checking (--overflow-check)

### Sub-Step 9.1: Add --overflow-check Flag

**After:** Add to `CodegenEmitterOptions.h`:
```cpp
bool overflow_check = false;
```

---

### Sub-Step 9.2: Use LLVM WithOverflow Intrinsics

**After:** In the LLVM backend's `operate()` function (for `+`, `-`, `*` on integers),
use the `*_with_overflow` LLVM intrinsic when `overflow_check` is enabled:

```cpp
// In compiler/backend/LLVM.cpp, in the binary operation codegen:
if(gen.overflow_check && is_arithmetic(op) && is_integer(type)) {
    llvm::Intrinsic::ID intrinsic_id;
    switch(op) {
        case Operation::Addition: intrinsic_id = llvm::Intrinsic::sadd_with_overflow; break;
        case Operation::Subtraction: intrinsic_id = llvm::Intrinsic::ssub_with_overflow; break;
        case Operation::Multiplication: intrinsic_id = llvm::Intrinsic::smul_with_overflow; break;
        default: goto normal_path;
    }
    auto result = gen.builder->CreateBinaryIntrinsic(intrinsic_id, lhs, rhs);
    auto value = gen.builder->CreateExtractValue(result, 0);
    auto overflow = gen.builder->CreateExtractValue(result, 1);
    
    // Branch: if overflow, trap
    auto trap_block = llvm::BasicBlock::Create(...);
    auto cont_block = llvm::BasicBlock::Create(...);
    gen.builder->CreateCondBr(overflow, trap_block, cont_block);
    
    gen.builder->SetInsertPoint(trap_block);
    gen.builder->CreateCall(/* llvm.trap */);
    gen.builder->CreateUnreachable();
    
    gen.builder->SetInsertPoint(cont_block);
    return value;
}
normal_path:
// Existing codegen...
```

**Validation:**
```bash
cat > lang/tests/safety/overflow_test.ch << 'EOF'
func test_overflow() {
    var max_i32 = 2147483647
    var result = max_i32 + 1  // Should trap with --overflow-check
}
EOF

cmake-build-debug/Compiler lang/tests/safety/overflow_test.ch -o lang/tests/safety/overflow_test.exe --mode debug_complete --overflow-check
./lang/tests/safety/overflow_test.exe
# EXPECTED: traps with SIGILL (calls llvm.trap on overflow)
```

---

## Feature 10: Lifetime Escape Analysis (Basic)

### Sub-Step 10.1: Track Scope Depth in SymResLinkBody

**Before:** No scope depth tracking.

**After:** Add a scope depth counter:

```cpp
// In SymResLinkBody.h
int scope_depth = 0;
```

Increment on scope entry, decrement on scope exit:
```cpp
// In table.scope_start() — track depth
void SymResLinkBody::VisitBlockScope(BlockScope* node) {
    table.scope_start();
    scope_depth++;
    for (const auto child: node->nodes) {
        visit(child);
    }
    table.scope_end();
    scope_depth--;
}
```

---

### Sub-Step 10.2: Track Reference Bindings

**After:** When a reference is assigned to a local variable, record its scope depth:

```cpp
// In SymResLinkBody.h
struct RefBinding {
    VariableIdentifier* var;
    int scope_depth;  // depth at which the referent was created
};
std::vector<RefBinding> ref_bindings;
```

When visiting a `VarInitStatement` where the type is a reference (`&T`), record it:
```cpp
// In VisitVarInitStmt, after resolving:
if(node->known_type()->is_reference() && safe_mode != SafeMode::Off) {
    ref_bindings.push_back({node, scope_depth});
}
```

---

### Sub-Step 10.3: Check Return of Reference to Local

**After:** In `VisitReturnStmt`, if the returned value is a reference, check that
it doesn't refer to a local variable:

```cpp
// In VisitReturnStmt:
if(node->value && node->value->getType()->is_reference()) {
    // Check if the reference refers to a local variable
    auto ref_id = /* extract the identifier being referenced */;
    for(auto& binding : ref_bindings) {
        if(binding.var->linked == ref_id && binding.scope_depth >= scope_depth) {
            // The referent is at the same or deeper scope — it will be destroyed!
            if(safe_mode == SafeMode::Enforce) {
                diagnoser.error("returning reference to local variable", node);
            } else if(safe_mode == SafeMode::Warn) {
                diagnoser.warn("returning reference to local variable", node);
            }
        }
    }
}
```

**Validation:**
```bash
cat > lang/tests/safety/lifetime_test.ch << 'EOF'
func dangling() : &int {
    var x = 42
    return &x  // Should error with --safe: returning ref to local
}

func valid() : &int {
    var x = 42
    // If x was passed in or is static, this is OK
    // This depends on the specific implementation
}
EOF
cmake-build-debug/TCCCompiler lang/tests/safety/lifetime_test.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: error about returning reference to local variable
```

---

### Sub-Step 10.4: Check Struct Field Holding Reference to Local

**After:** When a struct with a reference field is constructed, check that the
referent outlives the struct:

```cpp
// In VisitStructValue:
if(struct_type has reference fields) {
    for each reference field {
        if(the referent is a local at shallower depth) {
            error("struct holds reference to local variable");
        }
    }
}
```

---

## Feature 11: Full --safe Flag Architecture

### Sub-Step 11.1: Define SafeMode Enum

Create `compiler/SafeMode.h`:
```cpp
#pragma once
#include <cstdint>

enum class SafeMode : uint8_t {
    Off = 0,
    Warn = 1,
    Enforce = 2,
};
```

---

### Sub-Step 11.2: Wire --safe Through CLI

In `core/main/CompilerMain.cpp`, parse `--safe`:
```cpp
// In the argument parser:
else if(arg == "--safe" || arg == "--safe=enforce") {
    compiler_opts.safe_mode = SafeMode::Enforce;
} else if(arg == "--safe=warn") {
    compiler_opts.safe_mode = SafeMode::Warn;
}
```

---

### Sub-Step 11.3: Wire --safe Through Build Pipeline

Pass `safe_mode` from `LabBuildCompilerOptions` → `CodegenEmitterOptions` → codegen.

---

## Implementing a Full Feature: End-to-End Walkthrough

Below is a complete walkthrough of implementing **Feature 1.3 (Promote Deref Warning)** as an example of the methodology.

### Step-by-step AI Prompt

**Prompt for AI:**

```
I need to modify the Chemical compiler to promote the dereference warning to
an error when the --safe flag is enabled. Here's exactly what to do:

1. Read `compiler/symres/SymResLinkBody.h` — find the `safe_context` member
2. Read `compiler/symres/SymResLinkBody.cpp` — find `VisitDereferenceValue` around line 2388
3. The current code is:
   ```
   void SymResLinkBody::VisitDereferenceValue(DereferenceValue* value) {
       if(safe_context) {
           diagnoser.warn("de-referencing a pointer in safe context is prohibited", value);
       }
       ...
   }
   ```
4. Change it to:
   ```
   void SymResLinkBody::VisitDereferenceValue(DereferenceValue* value) {
       if(safe_context) {
           switch(safe_mode) {
               case SafeMode::Enforce:
                   diagnoser.error("...", value);
                   break;
               case SafeMode::Warn:
                   diagnoser.warn("...", value);
                   break;
               default:
                   break;
           }
       }
       ...
   }
   ```
5. Add the `SafeMode` enum to a new file `compiler/SafeMode.h`
6. Add `safe_mode` field to SymResLinkBody.h
7. Update the SymResLinkBody constructor

CONSTRAINTS:
- Do NOT modify any test files
- Do NOT remove any #include directives
- The SafeMode::Off default must be used when no flag is given
```

### Validation Script

```bash
#!/bin/bash
# validate_feature_1_3.sh
set -e

echo "=== Building compiler ==="
./scripts/build.sh --tcc

echo "=== Test 1: Safe code without flag (no diagnostic) ==="
cat > /tmp/test_deref.ch << 'EOF'
func main() {
    var x = 42
    var p = &raw x
    return *p
}
EOF
OUTPUT=$(cmake-build-debug/TCCCompiler /tmp/test_deref.ch -o /dev/null --mode debug_complete 2>&1)
if echo "$OUTPUT" | grep -qi "de-referencing"; then
    echo "FAIL: Got diagnostic without flag"
    exit 1
fi
echo "PASS"

echo "=== Test 2: Safe code with --safe=warn (warning) ==="
OUTPUT=$(cmake-build-debug/TCCCompiler /tmp/test_deref.ch -o /dev/null --mode debug_complete --safe=warn 2>&1) || true
if ! echo "$OUTPUT" | grep -qi "de-referencing"; then
    echo "FAIL: Expected warning with --safe=warn"
    exit 1
fi
echo "PASS"

echo "=== Test 3: Safe code with --safe (error) ==="
OUTPUT=$(cmake-build-debug/TCCCompiler /tmp/test_deref.ch -o /dev/null --mode debug_complete --safe 2>&1) || true
if ! echo "$OUTPUT" | grep -qi "de-referencing.*not allowed"; then
    echo "FAIL: Expected error with --safe"
    exit 1
fi
echo "PASS"

echo "=== Test 4: Unsafe block silences diagnostic ==="
cat > /tmp/test_deref_unsafe.ch << 'EOF'
func main() {
    var x = 42
    var p = &raw x
    unsafe {
        return *p
    }
}
EOF
OUTPUT=$(cmake-build-debug/TCCCompiler /tmp/test_deref_unsafe.ch -o /dev/null --mode debug_complete --safe 2>&1)
if echo "$OUTPUT" | grep -qi "de-referencing"; then
    echo "FAIL: Got diagnostic in unsafe block"
    exit 1
fi
echo "PASS"

echo "=== Test 5: Existing tests still pass ==="
./scripts/test.sh --tcc 2>&1
echo "PASS"

echo "=== All tests passed! ==="
```

---

## Feature Flag Architecture Summary

```
Flag                    | Scope          | Effect
------------------------|----------------|-------------------------------------------
--safe                  | Module/Global  | Enables all safety checks as errors
--safe=warn             | Module/Global  | Enables all safety checks as warnings only
--bounds-check          | Module/Global  | Inserts array bounds checks (debug mode)
--overflow-check        | Module/Global  | Inserts overflow checks (debug mode)
--check-null            | Module/Global  | Inserts null checks before deref
--enforce-unsafe        | Module/Global  | @unsafe function boundaries enforced
--enforce-extern-safe   | Module/Global  | Extern C calls require unsafe block
```

**In `chemical.mod`:**
```toml
[module]
safe = true
bounds-check = true
overflow-check = true
```

**Migration workflow:**
1. Start with `--safe=warn` — see all safety violations as warnings
2. Fix violations or add `unsafe { }` blocks where appropriate
3. Switch to `--safe` — violations become errors
4. Enable stricter checks one by one

---

## Safety Infrastructure Reference

### Key Files by Feature

| Feature | Primary File | Secondary Files |
|---------|-------------|-----------------|
| safe_context | `SymResLinkBody.cpp` | `SymResLinkBody.h` |
| @unsafe enforcement | `TypeVerify.cpp` | `TypeVerify.h` |
| Extern C check | `SymResLinkBody.cpp` | `FunctionDeclaration.h` |
| InterfaceBits | `InterfaceBits.h` | `CoreNodes.h`, `LinkSignature.cpp` |
| Bounds checking | `backend/LLVM.cpp` | `CodegenEmitterOptions.h` |
| Overflow checking | `backend/LLVM.cpp` | `CodegenEmitterOptions.h` |
| Lifetime analysis | `SymResLinkBody.cpp` | `TypeVerify.cpp` |
| Checked arithmetic | `lang/libs/core/ops.ch` | `lang/libs/std/src/` |

### How the Visitor Pattern Works

The compiler uses the **Visitor pattern** extensively. Each AST node has a corresponding
`Visit*` method. Adding a safety check means finding the right `Visit*` method and
inserting a check before or after the existing logic.

```
AST Node                Visitor Method              What to Check
──────────────────────────────────────────────────────────────────
DereferenceValue   →   VisitDereferenceValue    →   safe_context + safe_mode
FunctionCall       →   VisitFunctionCall        →   is_extern || is_unsafe
CastedValue        →   VisitCastedValue         →   target is *mut → safe_context
VarInitStatement   →   VisitVarInitStmt         →   reference type → scope depth
ReturnStatement    →   VisitReturnStmt           →   reference to local
Expression         →   VisitExpression           →   pointer arithmetic
Assignment         →   VisitAssignmentStmt       →   pointer arithmetic
```

### Diagnostic API

```cpp
// Use these methods on ASTDiagnoser:
diagnoser.error(message, node);         // Hard error — stops compilation
diagnoser.warn(message, node);           // Warning — continues compilation
diagnoser.error(node) << message;        // Stream-style error
diagnoser.warn(node) << message;         // Stream-style warning
```

---

## Comparison with Other Languages

| Feature | Chemical | Rust | Zig | C | 
|---------|----------|------|-----|---|
| Move semantics | ✅ Enforced | ✅ Borrow chk | ❌ | ❌ |
| Destructors (RAII) | ✅ Compiler-gen | ✅ Derive | ✅ `defer` | ❌ |
| Use-after-move error | ✅ Compile-time | ✅ Compile-time | N/A | ❌ |
| `unsafe` enforcement | ⚠️ Tracked but warn | ✅ Enforced | ✅ Enforced | N/A |
| `where` clause / trait bounds | ✅ Implemented | ✅ Full | ✅ `comptime` | N/A |
| InterfaceBits / marker traits | ✅ Partial | ✅ `auto trait` | ❌ | ❌ |
| Checked arithmetic (lib) | ❌ Missing | ✅ Std library | ✅ Wrapping by default | ❌ |
| Array bounds checking | ❌ Not implemented | ✅ Debug check | ✅ Debug check | ❌ |
| Null safety | ❌ Missing | ✅ `Option` | ❌ | ❌ |
| Integer overflow safety | ❌ UB | ✅ Debug check | ✅ Wrapping | ❌ |
| Reference lifetimes | ❌ Missing | ✅ Borrow checker | ❌ | ❌ |
| Feature flags for safety | ❌ Missing | ❌ | ✅ `-Doverflow` | ❌ |

---

*Document updated from comprehensive codebase analysis.
Last updated: July 18, 2026.*
