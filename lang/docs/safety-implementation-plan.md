# Chemical Safety Features — AI-Driven Implementation Plan

> **Status: Living Plan — Created July 18, 2026**
>
> This document defines **what** to build and **why**, with just enough **how** to keep an
> AI agent on the path to a complete, tested, performant safety system for Chemical.
>
> **Companion document:** `chemical-safety-analysis.md` — the analysis this plan is built from.
>
> **Core assumption:** Every feature is implemented by AI, validated by negative tests,
> and must not break existing passing tests. Existing libraries and test modules opt out
> of safety enforcement. New safety-aware modules opt in.

---

## Table of Contents

1. [Design Principles](#design-principles)
2. [Architecture: The Safety System](#architecture-the-safety-system)
3. [Prerequisites (Blockers Before Phase 1)](#prerequisites)
4. [Phase 0: Infrastructure](#phase-0-infrastructure)
5. [Phase 1: Unsafe Boundary Enforcement](#phase-1-unsafe-boundary-enforcement)
6. [Phase 2: Module-Level Safety Configuration](#phase-2-module-level-safety-configuration)
7. [Phase 3: Runtime Safety Checks](#phase-3-runtime-safety-checks)
8. [Phase 4: Container Safety](#phase-4-container-safety)
9. [Phase 5: Lifetime & Reference Safety](#phase-5-lifetime--reference-safety)
10. [Phase 6: Standard Library Safety Wrappers](#phase-6-standard-library-safety-wrappers)
11. [The Annotation Catalog](#the-annotation-catalog)
12. [Negative Testing Framework](#negative-testing-framework)
13. [Regression Protocol](#regression-protocol)
14. [AI Agent Operating Rules](#ai-agent-operating-rules)
15. [Feature-to-Phase Dependency Map](#feature-to-phase-dependency-map)

---

## Design Principles

These are non-negotiable. Every sub-step must satisfy all of them.

### P1: Zero Regression

**No existing test may fail.** The very first action in any sub-step is to run the
current test suite and record the baseline pass count. The very last action is to run
the same suite and confirm the same count. Any regression means the sub-step is reverted.

How: Every safety feature is gated behind a flag or option that defaults to OFF.
Without the flag, behavior is identical to before the change.

### P2: Opt-Out by Default (Not Opt-In)

Existing modules (std, core, tests, all libraries) must NOT be forced to adopt safety
features. They use `unsafe = true` (or equivalent) in their `chemical.mod` to opt out.
New modules get safety enforcement by default.

This means the default module safety level is **safe** (enforcement on). The existing
codebase explicitly opts out. This is the opposite of what the analysis document
suggests (it says "opt-in"), but it's the correct choice for a language that wants
safety as its default story.

### P3: Performant by Default

Safety checks must not add runtime cost to code that doesn't use safety features.
- Compile-time checks (move semantics, unsafe boundary): zero runtime cost.
- Runtime checks (bounds, overflow): only inserted when the module enables them.
- No heap allocations for safety metadata. Use arena-allocated bitflags and
  inline fields on existing AST nodes.
- Safety annotations on functions are checked at compile time only.

### P4: Small, Atomic Changes

Every sub-step is designed so that:
- It modifies at most 3 files.
- It changes at most 30 lines of C++.
- It is independently testable with a single `.ch` file.
- It has a clear "before" and "after" state that an AI can search for.
- If the sub-step breaks something, reverting it is trivial.

### P5: Negative Tests Are the Source of Truth

Positive tests (code that should compile) verify the language works.
Negative tests (code that should NOT compile) verify safety features work.
Without negative tests, safety features are untested.

Every sub-step that adds a safety check MUST add a corresponding negative test
in `lang/tests/safety/negative/` and a positive test in `lang/tests/safety/positive/`.

### P6: Separation of Concerns

Each safety subsystem is a self-contained component:
- The annotation system defines what operations are unsafe.
- The boundary checker enforces unsafe/safe boundaries.
- The runtime checker inserts runtime safety checks.
- The container checker validates type constraints.
- The lifetime checker tracks reference validity.

Each subsystem has its own:
- Data structures (stored on existing AST nodes, not new heaps).
- Visitor methods in the appropriate compiler pass.
- Test files in `lang/tests/safety/`.
- Feature flag or module option.

---

## Architecture: The Safety System

### Where Safety Checks Live

```
                    ┌──────────────────────────────────┐
                    │         chemical.mod              │
                    │   (module safety options)         │
                    └────────────┬─────────────────────┘
                                 │
                    ┌────────────▼─────────────────────┐
                    │      LabBuildCompilerOptions      │
                    │   (global + per-module flags)     │
                    └────────────┬─────────────────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         │                       │                       │
┌────────▼────────┐   ┌─────────▼────────┐   ┌─────────▼────────┐
│  SymResLinkBody  │   │   TypeVerify     │   │   LLVM / TCC     │
│  (compile-time   │   │  (compile-time   │   │  (runtime checks)│
│   boundary       │   │   type-level     │   │   bounds, overflow│
│   checks)        │   │   checks)        │   │   null checks)   │
└──────────────────┘   └──────────────────┘   └──────────────────┘
```

### The Safety Level Hierarchy

```
Module-level (chemical.mod)
  └── applies to all source files in the module
  └── can be overridden by --safe/--unsafe CLI flags

Function-level (@unsafe annotation)
  └── the function's body is treated as unsafe context
  └── callers must be in unsafe block to call it

Block-level (unsafe { } blocks)
  └── within a function, individual blocks can be marked unsafe
  └── temporary escape hatch for specific operations
```

### The `SafeMode` Enum (Central to Everything)

```cpp
enum class SafeMode : uint8_t {
    Off = 0,      // No safety enforcement (default for --safe not passed)
    Warn = 1,     // Emit warnings, don't error
    Enforce = 2,  // Emit errors for violations
};
```

This enum flows through:
- `LabBuildCompilerOptions.safe_mode` — global, from CLI `--safe` flag.
- `ModuleFileData.safe_mode` — per-module, from `chemical.mod`.
- `SymResLinkBody.safe_mode` — used during body linking to check unsafe boundaries.
- `TypeVerifier.safe_mode` — used during type verification.
- `LLVMBackendContext.safe_mode` — used during codegen for runtime check insertion.

---

## Prerequisites

These must be completed before Phase 1 begins. They are blockers.

### PR1: Negative Test Infrastructure

**What:** A shell-based test runner that compiles `.ch` files and checks exit code + stderr.

**Why:** Without this, every subsequent safety feature has no automated test.

**Files:**
- Create `lang/tests/safety/run_safety_tests.sh`
- Create directory structure: `lang/tests/safety/positive/`, `lang/tests/safety/negative/`
- Create `lang/tests/safety/config/test_list.txt`

**Validation:**
```bash
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler
# Should show 0 tests initially, exit 0
```

**Sub-steps:**
1. Create directory structure.
2. Write the shell script with `run_positive()`, `run_negative()`, `run_runtime_trap()`.
3. Wire it into `./scripts/test.sh` as an optional `--safety` flag.
4. Verify it runs clean with no tests registered.

### PR2: Module Options Parsing in `chemical.mod`

**What:** Allow `chemical.mod` files to declare arbitrary key=value options.

**Why:** Every module needs to declare its safety level. The mechanism must be generic
so it can support future options too.

**Files to modify:**
- `parser/Parser.cpp` (add `var` / `option` keyword handling in `parseModuleFile`)
- `compiler/processor/ModuleFileData.h` (add `std::unordered_map<chem::string_view, chem::string_view> options`)
- `compiler/lab/mod_conv/ModToLabConverter.cpp` (emit option-setting code)
- `compiler/lab/LabBuildCompiler.cpp` (read options in `build_module_from_mod_file`)

**Syntax in `chemical.mod`:**
```
application my_app
source "src"

// Safety options
option safe = enforce
option bounds-check = true
```

**Or a simpler form using existing `var`/`const` parsing:**
```
application my_app
source "src"

var safe = "enforce"
var bounds-check = "true"
```

The `var` form leverages existing parser infrastructure (there's already a TODO for
handling `var` in `parseModuleFile`). Values are always strings; the consuming code
parses them.

**Validation:**
```bash
# Create test mod file with option
echo 'application test_mod
source "src"
var safe = "enforce"' > /tmp/test_opt/chemical.mod
mkdir -p /tmp/test_opt/src
echo 'func main() {}' > /tmp/test_opt/src/main.ch

# Verify the option is parsed (add a diagnostic or verbose output)
cmake-build-debug/TCCCompiler /tmp/test_opt/chemical.mod -v 2>&1 | grep "safe"
# Should show the option value
```

### PR3: `SafeMode` Plumbing Through Compilation Pipeline

**What:** Create `compiler/SafeMode.h`, add `SafeMode safe_mode` to all relevant
options and visitor classes, wire it from CLI `--safe` flag through to symres/typeverify.

**Files to modify:**
- Create `compiler/SafeMode.h` (the enum + utility functions)
- `compiler/lab/LabBuildCompilerOptions.h` (add `SafeMode safe_mode = SafeMode::Off`)
- `compiler/symres/SymResLinkBody.h` (add `SafeMode safe_mode = SafeMode::Off`)
- `compiler/symres/SymResLinkBody.cpp` (initialize from options)
- `compiler/typeverify/TypeVerify.h` (add `SafeMode safe_mode = SafeMode::Off`)
- `compiler/typeverify/TypeVerify.cpp` (initialize from options)
- `core/main/CompilerMain.cpp` (parse `--safe` / `--safe=warn` / `--safe=enforce`)

**CLI syntax:**
```
--safe              → SafeMode::Enforce
--safe=warn         → SafeMode::Warn
--safe=enforce      → SafeMode::Enforce
--safe=off          → SafeMode::Off (default)
```

**Validation:**
```bash
./scripts/build.sh --tcc
# Must compile without errors
# No behavior change yet — the flag exists but nothing reads it
```

### PR4: Expose `--safe` as `release_safe` Mode

**What:** Ensure `--mode release_safe` (which already exists in `OutputMode`) sets
`SafeMode::Enforce` on all compilation phases.

**Why:** Users already know `-mode` flag. This gives them another way to enable safety.

**Files:**
- `compiler/lab/LabBuildContext.cpp` (when mode is `ReleaseSafe`, set `safe_mode = Enforce`)
- `core/main/CompilerMain.cpp` (when parsing `--mode release_safe`, set safe_mode)

---

## Phase 0: Infrastructure

Phase 0 establishes the testing and configuration infrastructure. No safety features
are implemented yet — just the scaffolding.

### Step 0.1: Create the Safety Test Runner

**Task:** Create `lang/tests/safety/run_safety_tests.sh` and its directory structure.

**Sub-steps:**
1. Create directories: `lang/tests/safety/{positive,negative,runtime,config}/`
2. Create `run_safety_tests.sh` with three runner functions:
   - `run_positive(test_file, flags)` — compile, expect exit 0
   - `run_negative(test_file, flags, error_pattern)` — compile, expect non-zero + stderr match
   - `run_runtime_trap(test_file, compiler_flags)` — compile, run, expect non-zero exit
3. Create `config/test_list.txt` — empty initially
4. Wire into `./scripts/test.sh` with `--safety` flag
5. Add a "no tests" default path that exits 0

**Test the infrastructure itself:**
- Write a trivial positive test (`func main() {}`) — should pass
- Write a trivial negative test (syntax error) — should fail with expected error
- Verify both work

### Step 0.2: Create `SafeMode.h` and Wire Through Pipeline

**Task:** The plumbing that makes all safety features possible.

**Sub-steps:**
1. Create `compiler/SafeMode.h` with the `SafeMode` enum
2. Add `SafeMode safe_mode = SafeMode::Off` to `LabBuildCompilerOptions`
3. Parse `--safe` / `--safe=warn` / `--safe=enforce` in `CompilerMain.cpp`
4. Pass `safe_mode` from options into `SymResLinkBody` constructor
5. Pass `safe_mode` from options into `TypeVerifier`
6. Add a `check_safe(message, node)` helper to `SymResLinkBody` that does the
   warn/emit-error switch

**Validation:**
- Build compiles
- `--safe` flag is accepted by CLI
- No behavior change (the flag exists but nothing reads it yet)

### Step 0.3: Module Options in `chemical.mod`

**Task:** Allow modules to declare `var safe = "enforce"` (or similar) in `chemical.mod`.

**Sub-steps:**
1. Add `std::unordered_map<chem::string_view, chem::string_view> options` to `ModuleFileData`
2. Handle `var` keyword in `BasicParser::parseModuleFile()` — parse `var name = "value"` 
   into the options map
3. In `ModToLabConverter`, emit option-setting code for recognized keys
4. In `LabBuildCompiler::build_module_from_mod_file()`, read options from parsed data
5. Create a `ModuleSafetyConfig` struct that reads `safe`, `bounds-check`, `overflow-check`
   from the options map

**Validation:**
- A `chemical.mod` with `var safe = "enforce"` parses without error
- A `chemical.mod` with unknown `var` values also parses (unknown options ignored with warning)

---

## Phase 1: Unsafe Boundary Enforcement

Phase 1 implements the core unsafe/safe boundary: operations that are only allowed
inside `unsafe { }` blocks or `@unsafe` functions.

### The Concept

Chemical has two kinds of "unsafe context":
1. **`unsafe { }` blocks** — explicit escape hatch within a function
2. **`@unsafe` functions** — the entire function body is unsafe context

When `SafeMode::Enforce` is active:
- Pointer dereference outside unsafe context → error
- Pointer arithmetic outside unsafe context → error
- Cast to pointer from non-pointer outside unsafe context → error
- `dealloc` outside unsafe context → error
- Calling `@unsafe` function outside unsafe context → error
- Calling extern C function outside unsafe context → error

When `SafeMode::Warn` is active:
- Same operations → warning (not error)

When `SafeMode::Off`:
- No diagnostics (current behavior)

### Step 1.1: Promote Deref Warning to Error

**Task:** Change `VisitDereferenceValue` in `SymResLinkBody.cpp` from always-warn to
conditional based on `safe_mode`.

**Before (current code at `SymResLinkBody.cpp:2387`):**
```cpp
if(safe_context) {
    diagnoser.warn("de-referencing a pointer in safe context is prohibited", value);
}
```

**After:**
```cpp
if(safe_context && safe_mode != SafeMode::Off) {
    check_safe("de-referencing a pointer outside unsafe block is not allowed", value);
}
```

**Negative test:** `lang/tests/safety/negative/deref_in_safe.ch`
```chemical
func main() {
    var x = 42
    var p = &raw x
    *p  // should error with --safe
}
```

**Positive test:** `lang/tests/safety/positive/deref_in_unsafe.ch`
```chemical
func main() {
    unsafe {
        var x = 42
        var p = &raw x
        *p  // OK: inside unsafe block
    }
}
```

**Regression:** `./scripts/test.sh --tcc --no-build` — all existing tests must pass.

### Step 1.2: Check `dealloc` in Safe Context

**Task:** Add safety check to `VisitDeallocStmt` in `SymResLinkBody.cpp`.

**Before (current code):**
```cpp
void SymResLinkBody::VisitDeallocStmt(DeallocStmt* node) {
    visit(node->ptr);
}
```

**After:**
```cpp
void SymResLinkBody::VisitDeallocStmt(DeallocStmt* node) {
    if(safe_context && safe_mode != SafeMode::Off) {
        check_safe("deallocating memory requires an unsafe block", node);
    }
    visit(node->ptr);
}
```

**Negative test:** Code with `dealloc` outside unsafe block.
**Positive test:** Code with `dealloc` inside unsafe block.

### Step 1.3: Check Pointer Arithmetic in Safe Context

**Task:** Detect binary operations on pointer types and require unsafe context.

**Where:** This needs to be checked during expression visiting. When a binary expression
has a pointer operand (`+`, `-`, `++`, `--`), and the context is safe, emit a diagnostic.

**Approach:** In `SymResLinkBody` expression handling, after resolving the expression
type, check if either operand is a pointer type and the operation is arithmetic.

**Negative test:** `ptr + 1` outside unsafe block.
**Positive test:** `ptr + 1` inside unsafe block.

### Step 1.4: Check Cast-to-Pointer in Safe Context

**Task:** In `VisitCastedValue`, check if the target type is a pointer.

**Where:** `SymResLinkBody.cpp` — find the cast visitor method.

**Negative test:** `42 as *int` outside unsafe block.
**Positive test:** `42 as *int` inside unsafe block.

### Step 1.5: Uncomment and Fix `@unsafe` Function Call Check

**Task:** Re-enable the commented-out check in `TypeVerify.cpp:454`.

**Before (current code at `TypeVerify.cpp:454`):**
```cpp
// if(func_decl->is_unsafe() && resolver.safe_context) {
//     resolver.error("unsafe function with name should be called in an unsafe block", this);
// }
```

**After:**
```cpp
if(func_decl && func_decl->is_unsafe() && !verifier.is_unsafe && verifier.safe_mode != SafeMode::Off) {
    switch(verifier.safe_mode) {
        case SafeMode::Enforce:
            verifier.diagnoser.error("calling @unsafe function requires an unsafe block", node);
            break;
        case SafeMode::Warn:
            verifier.diagnoser.warn("calling @unsafe function from safe context", node);
            break;
        default: break;
    }
}
```

**Note:** The TypeVerifier already has `is_unsafe` (line 48 of TypeVerify.h) that toggles
in `VisitUnsafeBlock`. So the infrastructure is ready — we just need to uncomment and
update the check.

**Negative test:** Call `@unsafe func` without `unsafe` block, with `--safe`.
**Positive test:** Call `@unsafe func` inside `unsafe` block, with `--safe`.
**Regression:** Mark all existing `@unsafe` functions in std/core libraries as safe-to-call
by wrapping their call sites or by having those modules opt out of safety.

### Step 1.6: Check Extern C Function Calls in Safe Context

**Task:** When a function call resolves to an extern declaration, require unsafe context.

**Where:** `SymResLinkBody::VisitFunctionCall` — after resolving the callee.

**Approach:**
```cpp
if(safe_context && safe_mode != SafeMode::Off) {
    auto callee = /* get linked function */;
    if(callee && callee->is_extern_fn()) {
        check_safe("calling extern C function requires an unsafe block", call);
    }
}
```

**Negative test:** `printf("hello")` outside unsafe block with `--safe`.
**Positive test:** `printf("hello")` inside unsafe block with `--safe`.

**Regression consideration:** ALL existing test code that calls `printf` must either:
- Be in modules that opt out of safety (`var safe = "off"`), OR
- Have their `printf` calls wrapped in `unsafe { }`, OR
- The check must be a warning in Phase 1, not an error

**Decision:** In Phase 1, extern check is **warn-only** even in Enforce mode, because
every test file uses `printf`. Enforcement begins in Phase 2 when modules can opt out.

### Step 1.7: Exempt Standard Library Modules

**Task:** Ensure that `core`, `std`, and all test modules have safety enforcement OFF.

**Where:** In each library's `chemical.mod`, add `var safe = "off"`.
In each test module's `build.lab`, set `ctx.set_safe_mode(job, SafeMode::Off)`.

**Files to modify:**
- `lang/libs/core/chemical.mod` — add `var safe = "off"`
- `lang/libs/std/chemical.mod` — add `var safe = "off"`
- `lang/tests/build.lab` — set safe mode off for all test modules
- All other library `chemical.mod` files — add `var safe = "off"`

**Validation:**
```bash
./scripts/test.sh --tcc
# All existing tests must pass (they already compile without --safe)
# With --safe, test modules should still compile because they opt out
```

### Step 1.8: The `@unsafe` "Cannot Be Called in Safe Mode" Annotation

**Task:** This is a new annotation, distinct from `@unsafe`. Where `@unsafe` means
"a function that contains unsafe operations," this new annotation means "a function
that MUST NOT be called from safe context under any circumstances — even wrapping
the call in `unsafe { }` does not help."

**Use case:** `malloc`, `free`, `realloc` — these are C stdlib functions that are
fundamentally unsafe and should never be called from safe Chemical code. Even if
the caller wraps in `unsafe`, the compiler should still reject it.

**Proposed annotation:** `@unsafe_call` or `@forbid_safe`

Wait — re-reading the requirement: "annotation to mark a function, so it can't be
called in unsafe mode, leads to a compile time error, because it is completely unsafe"

This means: the function IS callable, but ONLY from code that explicitly opts into
unsafe at the module level or function level. The annotation marks functions that
are too dangerous for normal `unsafe { }` blocks.

**Revised interpretation:** `@unsafe` already means "requires unsafe block to call."
The user wants a DIFFERENT annotation for functions that are SO unsafe they should
trigger an error even inside `unsafe { }` blocks.

**Proposed annotation:** `@unsafe("critical")` — a severity parameter on the existing
`@unsafe` annotation. Functions marked `@unsafe("critical")` can only be called from
functions that are themselves marked `@unsafe("critical")`. A regular `unsafe { }`
block is not sufficient.

**Implementation:**
1. Extend `annot_handler_unsafe` to accept an optional string argument
2. Store the severity level on `FunctionDeclaration` (e.g., `UnsafeLevel::Normal`
   vs `UnsafeLevel::Critical`)
3. In the call-site check, if the callee is `Critical` and the caller is not
   `Critical`, emit error regardless of `unsafe { }` blocks

**Files to modify:**
- `compiler/frontend/AnnotationController.cpp` — extend `annot_handler_unsafe`
- `ast/structures/FunctionDeclaration.h` — add `UnsafeLevel` enum and field
- `compiler/typeverify/TypeVerify.cpp` — check severity at call sites

**Example usage:**
```chemical
@unsafe("critical")
@extern public func malloc(size : usize) : *void

@unsafe("critical")
@extern public func free(ptr : *void) : void

func main() {
    unsafe {
        var p = malloc(100)  // ERROR: malloc requires critical unsafe context
    }
}
```

**This requires that the caller function itself be `@unsafe("critical")`.**
A regular `unsafe { }` block is not sufficient.

**Regression:** Mark `malloc`, `free`, `realloc`, `calloc` in cstd with this annotation.
These are already in the `cstd` or `std` module which opts out of safety, so no existing
code breaks.

---

## Phase 2: Module-Level Safety Configuration

Phase 2 makes safety per-module, not global. This is where the real power comes from.

### Step 2.1: Read Module Safety Options

**Task:** During module compilation, read `var safe = "..."` from the module's parsed
`ModuleFileData` and set the `SafeMode` for all source files in that module.

**Where:** `LabBuildCompiler::build_module_from_mod_file()` and `ASTProcessor::compile_module()`.

**Flow:**
```
chemical.mod parsed → ModuleFileData.options["safe"] → "enforce"
                    → ModuleSafetyConfig.safe_mode = SafeMode::Enforce
                    → passed to all SymResLinkBody instances for this module
```

**Key decision:** The module's safety level applies to ALL source files in that module.
Individual functions can opt out with `unsafe { }` blocks. But the default within the
module is safe.

### Step 2.2: CLI Flags Override Module Options

**Task:** Ensure `--safe` and `--unsafe` CLI flags override per-module settings.

**Rules:**
- `--safe` on CLI → ALL modules use `SafeMode::Enforce` (regardless of their `chemical.mod`)
- `--safe=warn` on CLI → ALL modules use `SafeMode::Warn`
- `--unsafe` on CLI → ALL modules use `SafeMode::Off` (regardless of their `chemical.mod`)
- No flag → each module uses its own `chemical.mod` setting (or default)

**This means:** Users can always force safety on or off globally for testing.

### Step 2.3: The Default Module Safety Level

**Task:** Decide and implement what happens when a `chemical.mod` has NO `var safe = "..."`.

**Decision:** Default is `SafeMode::Enforce`. This means:
- New projects get safety by default
- Existing projects must add `var safe = "off"` to their `chemical.mod`
- This is the opposite of the analysis doc's suggestion, but correct for the goal

**Wait — reconsidering:** The analysis doc says features should default to OFF to
avoid breaking existing code. But the user says "existing libraries and test modules
would use these options to opt out." So the plan is:

**Final decision:** Default is `SafeMode::Off` (no enforcement). Modules explicitly
opt in with `var safe = "enforce"`. This preserves all existing behavior. The
standard library and test modules don't need changes. New modules that want safety
add one line to their `chemical.mod`.

### Step 2.4: Opt Out for All Existing Modules

**Task:** Add `var safe = "off"` to every existing library and test module.

**Files to modify:**
- `lang/libs/core/chemical.mod`
- `lang/libs/std/chemical.mod`
- `lang/libs/test/chemical.mod`
- `lang/libs/test_env/chemical.mod`
- `lang/libs/json/chemical.mod`
- `lang/libs/net/chemical.mod`
- `lang/libs/fs/chemical.mod`
- `lang/libs/html_cbi/chemical.mod`
- `lang/libs/css_cbi/chemical.mod`
- `lang/libs/js_cbi/chemical.mod`
- `lang/libs/universal_cbi/chemical.mod`
- `lang/libs/page/chemical.mod`
- `lang/tests/*/chemical.mod`
- All compiled app `chemical.mod` files

**Validation:**
```bash
# After adding opt-outs, run the full test suite
./scripts/test.sh --tcc
# All tests must still pass
```

### Step 2.5: Verify Module Opt-In Works

**Task:** Create a test project that opts into safety and verify enforcement.

**Create:**
```
lang/tests/safety/positive/safe_module_optin/
├── chemical.mod    # var safe = "enforce"
└── src/
    └── main.ch     # safe code that should compile
```

**Create:**
```
lang/tests/safety/negative/safe_module_optin/
├── chemical.mod    # var safe = "enforce"
└── src/
    └── main.ch     # unsafe code without unsafe block — should fail
```

**Validate:**
```bash
# The safe module should compile its safe code
cmake-build-debug/TCCCompiler lang/tests/safety/positive/safe_module_optin/chemical.mod -o /dev/null
# Expected: exit 0

# The unsafe code in the safe module should fail
cmake-build-debug/TCCCompiler lang/tests/safety/negative/safe_module_optin/chemical.mod -o /dev/null 2>&1
# Expected: exit non-zero, error about dereference
```

---

## Phase 3: Runtime Safety Checks

Phase 3 adds runtime checks that trap at execution time. These are separate from
compile-time boundary checks — they catch bugs at runtime.

### Step 3.1: Bounds Checking

**Task:** When `--bounds-check` is passed (or module has `var bounds-check = "true"`),
insert array bounds checks before every array indexing operation.

**Where:** `compiler/backend/LLVM.cpp` (for LLVM backend) and the TCC/C translation
backend.

**LLVM approach:**
- In `IndexOperator::llvm_value()` and `IndexOperator::llvm_pointer()`
- Before the GEP instruction, insert:
  ```
  %in_bounds = icmp ult %index, %array_size
  br i1 %in_bounds, label %cont, label %trap
  trap: call void @llvm.trap() unreachable
  cont: ; continue with GEP
  ```

**TCC/C approach:**
- In the C translation, insert `if(index >= size) __builtin_trap();` before the access.

**Negative test:** Array access out of bounds → program traps (exit code > 128).
**Positive test:** Array access in bounds → program runs normally.

### Step 3.2: Overflow Checking

**Task:** When `--overflow-check` is passed, use LLVM `*_with_overflow` intrinsics
for `+`, `-`, `*` on integer types.

**Where:** `compiler/backend/LLVM.cpp` — in binary operation codegen.

**LLVM approach:**
- Replace `add i32 %a, %b` with `%result = call { i32, i1 }
  @llvm.sadd.with.overflow.i32(i32 %a, i32 %b)`
- Extract the overflow bit
- If overflow → trap

**Negative test:** `INT_MAX + 1` → program traps.
**Positive test:** `1 + 1` → program runs normally.

### Step 3.3: Null Pointer Checking (Optional, Lower Priority)

**Task:** When dereferencing a pointer that is known to be nullable, insert a null check.

**Where:** `compiler/backend/LLVM.cpp` — in dereference codegen.

**Approach:** Only check pointers that come from nullable sources (e.g., `get_ptr()`
which can return null). Static non-nullable pointers don't need checks.

**This is AI-hard** because it requires tracking which pointers are nullable through
the type system. Defer to Phase 5.

---

## Phase 4: Container Safety

Phase 4 fixes the critical bugs in container operations that cause double-free and
use-after-free.

### Step 4.1: Restrict `vector<T>.get()` to Non-Destructible Types

**Task:** Make `get()` (which returns by value) only available when `T` does NOT have
a destructor. For destructible `T`, users must use `get_ptr()` or `get_ref()`.

**Where:** `lang/libs/std/src/vector.ch` — the `get` function.

**Approach 1 (preferred):** Use the existing `where` clause system:
```chemical
func get(&self, index : size_t) : T where T : Copy {
    return data_ptr[index]
}
```

If `T : Copy` is satisfied, `get()` works. If not, the user gets a clear error:
"get() requires T to implement Copy. Use get_ptr() for non-copyable types."

**Approach 2:** If `where` clauses on std library functions aren't yet enforced
during generic instantiation, add a compile-time check in the generic body that
errors if `T` has a destructor.

**Validation:**
```chemical
// This should work (int is Copy):
var v = std::vector<int>()
v.push(42)
var x = v.get(0)  // OK

// This should fail (string is not Copy):
var s = std::vector<std::string>()
s.push(std::string("hello"))
var y = s.get(0)  // ERROR: get() requires Copy
```

### Step 4.2: Add `where` Constraints to `vector.push()`

**Task:** Ensure `push()` only works when `T` is safely copyable (e.g., `Copy`).

**Where:** `lang/libs/std/src/vector.ch`

```chemical
func push(&mut self, value : T) where T : Copy {
    // existing implementation
}
```

### Step 4.3: Add `where` Constraints to `vector.resize()`

**Task:** Ensure `resize()` only works when `T` is default-constructible.

```chemical
func resize(&mut self, new_size : size_t) where T : DefaultConstructible {
    // existing implementation using zeroed<T>()
}
```

### Step 4.4: Add `InterfaceBits` for Destructible, TriviallyRelocatable

**Task:** Extend the `InterfaceBits` system with new bit constants.

**Where:** `ast/base/InterfaceBits.h`

```cpp
static constexpr BitsType COPY_BIT                  = BitsType(1) << 0;
static constexpr BitsType DESTRUCTIBLE_BIT          = BitsType(1) << 1;
static constexpr BitsType DEFAULT_CONSTRUCTIBLE_BIT = BitsType(1) << 2;
static constexpr BitsType TRIVIALLY_RELOCATABLE_BIT = BitsType(1) << 3;
```

**Auto-derive logic:** In symres, after processing a struct's members:
- If it has a destructor → set `DESTRUCTIBLE_BIT`
- If it has no destructor and is zeroable → set `DEFAULT_CONSTRUCTIBLE_BIT`
- If it has no destructor and all fields are trivially relocatable → set `TRIVIALLY_RELOCATABLE_BIT`

**Define corresponding interfaces** in `lang/libs/core/interfaces.ch`:
```chemical
public interface Destructible {}
public interface DefaultConstructible {}
public interface TriviallyRelocatable {}
```

**Register in `CoreNodes.h`** so the compiler can auto-implement them.

---

## Phase 5: Lifetime & Reference Safety

Phase 5 is the hardest and most impactful. It prevents dangling references.

### Step 5.1: Basic Reference Scope Tracking

**Task:** Track which scope each reference was created in, and prevent returning
references to local variables.

**Where:** `compiler/symres/SymResLinkBody.h` — add scope depth tracking.

**Data:**
```cpp
int scope_depth = 0;
struct RefBinding {
    ASTNode* referent;     // what the reference points to
    int scope_depth;       // depth at which referent was created
};
std::vector<RefBinding> ref_bindings;
```

**Check in `VisitReturnStmt`:**
```cpp
if(return_value is a reference) {
    for(auto& binding : ref_bindings) {
        if(binding.referent == return_value.referent && binding.scope_depth >= current_function_scope_depth) {
            diagnoser.error("returning reference to local variable", node);
        }
    }
}
```

### Step 5.2: One Mutable or Many Immutable References

**Task:** Enforce that at any point, there is either one `&mut` reference or
multiple `&` references to the same data — never multiple `&mut`.

**This is AI-hard.** It requires tracking all live references and their
mutability at every point in the function body. Defer to a later iteration.

### Step 5.3: Reference Storage in Structs

**Task:** Prevent storing a reference in a struct if the reference would outlive
the referent.

**This is AI-hard.** Defer to a later iteration.

---

## Phase 6: Standard Library Safety Wrappers

Phase 6 adds safe alternatives to unsafe standard library functions.

### Step 6.1: Checked Arithmetic Interfaces

**Task:** Define `CheckedAdd`, `CheckedSub`, `CheckedMul` interfaces and implement
them for all integer types.

**Where:** `lang/libs/core/ops.ch` (interfaces) and `lang/libs/std/src/` (implementations)

### Step 6.2: `NonNull<T>` Wrapper

**Task:** Create a `NonNull<T>` type that wraps a pointer and guarantees non-null.

**Where:** `lang/libs/std/src/ptr.ch` (new file)

### Step 6.3: Safe `vector` API

**Task:** Ensure the full `vector` API uses `where` clauses consistently:
- `get()` requires `Copy`
- `get_ptr()` works for all types
- `get_ref()` works for all types
- `push()` requires `Copy` or explicit move
- `resize()` requires `DefaultConstructible`
- `reserve()` works for all types

---

## The Annotation Catalog

### Existing Annotations (Relevant to Safety)

| Annotation | Current Effect | Safety-Relevant? |
|-----------|---------------|-----------------|
| `@unsafe` | Sets `func->is_unsafe = true` | Yes — caller requires unsafe context |
| `@extern` | Marks function as extern C | Yes — calling requires unsafe context |
| `@allow_zeroed` | Allows `zeroed<T>()` for types with ctors/dtors | Yes — bypasses zero-init safety |

### New Annotations to Add

| Annotation | Effect | Example |
|-----------|--------|---------|
| `@unsafe("critical")` | Function can only be called from `@unsafe("critical")` functions, not from regular `unsafe { }` blocks | `@unsafe("critical") @extern public func malloc(...) : *void` |
| `@safe_pure` | Function is guaranteed to have no side effects and no unsafe operations. Can be called from any context. | `@safe_pure func add(a : int, b : int) : int` |
| `@safe_abi` | Function has a safe ABI boundary — it can be called from safe code even if it's implemented in an unsafe module | For FFI wrappers |

### Annotation Implementation Pattern

Every new annotation follows this pattern:
1. Add handler function in `AnnotationController.cpp`
2. Add storage field on the relevant AST node (e.g., `FunctionDeclaration`)
3. Add the annotation name to `initialize()` registration
4. Add a check in the appropriate compiler pass (symres or typeverify)
5. Add positive and negative tests

---

## Negative Testing Framework

### Directory Structure

```
lang/tests/safety/
├── run_safety_tests.sh
├── config/
│   └── test_list.txt
├── positive/
│   ├── deref_in_unsafe.ch
│   ├── extern_in_unsafe.ch
│   ├── safe_by_default.ch
│   ├── safe_module_optin/
│   │   ├── chemical.mod
│   │   └── src/main.ch
│   └── expected_output/
├── negative/
│   ├── deref_in_safe.ch
│   ├── extern_call_no_unsafe.ch
│   ├── unsafe_fn_call.ch
│   ├── critical_unsafe_call.ch
│   ├── ptr_arith_in_safe.ch
│   ├── cast_to_ptr_in_safe.ch
│   ├── dealloc_in_safe.ch
│   ├── return_ref_to_local.ch
│   ├── safe_module_unsafe_code/
│   │   ├── chemical.mod
│   │   └── src/main.ch
│   └── expected_errors/
│       ├── deref_in_safe.txt
│       ├── extern_call_no_unsafe.txt
│       └── ...
└── runtime/
    ├── oob_test.ch
    ├── overflow_test.ch
    └── expected_behavior/
```

### How Negative Tests Work

Each negative test is a complete `.ch` file with a `chemical.mod` (or uses a default).
The test runner:
1. Compiles the file with the appropriate flags
2. Checks that compilation fails (exit code != 0)
3. Checks that stderr contains the expected error pattern
4. Reports pass/fail

**Example negative test (`deref_in_safe.ch`):**
```chemical
// chemical.mod: application deref_test, source "src"
// With --safe flag:
func main() {
    var x = 42
    var p = &raw x
    *p  // should error: "de-referencing a pointer outside unsafe block"
}
```

**Expected error pattern (`expected_errors/deref_in_safe.txt`):**
```
de-referencing a pointer outside unsafe block
```

### How to Run Negative Tests

```bash
# Run all safety tests
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler

# Run with verbose output
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler --verbose

# Run specific test category
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler --negative-only
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler --positive-only
```

### Adding a New Safety Test (Step by Step)

1. **Determine the test type:**
   - Code that SHOULD compile → positive test
   - Code that SHOULD NOT compile → negative test
   - Code that SHOULD trap at runtime → runtime test

2. **Create the `.ch` file** in the appropriate directory

3. **Create the expected output/error file** if needed

4. **Register in `test_list.txt`:**
   ```
   # test_name test_type flags expected_pattern
   deref_in_safe negative --safe "de-referencing"
   deref_in_unsafe positive --safe
   oob_test runtime "--bounds-check" ""
   ```

5. **Run the test:**
   ```bash
   bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler
   ```

---

## Regression Protocol

### Before Every Sub-Step

```bash
# 1. Record baseline
./scripts/test.sh --tcc 2>&1 | tee /tmp/baseline.txt
# Record: "Total X, Passed Y, Failed Z"

# 2. Run safety tests
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler 2>&1 | tee /tmp/safety_baseline.txt
# Record: "N passed, M failed"
```

### After Every Sub-Step

```bash
# 1. Build the compiler
./scripts/build.sh --tcc

# 2. Run safety tests (fast, ~2 seconds)
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler

# 3. Run full test suite (slower, ~30 seconds)
./scripts/test.sh --tcc --no-build

# 4. Compare results
# - Safety tests: new tests should pass
# - Full test suite: same pass count as baseline
# - If any existing test fails → REVERT the sub-step
```

### The Revert Protocol

If a sub-step breaks existing tests:
1. Identify the exact change that caused the regression
2. Revert that specific change (not the entire sub-step if possible)
3. Re-run to confirm the regression is gone
4. Re-analyze why the change broke tests
5. Fix the issue (usually: the safety check is firing for code that should be exempt)
6. Re-run the full validation

---

## AI Agent Operating Rules

### Rule 1: Never Modify Existing Test Files

AI must never modify `.ch` files in `lang/tests/src/`, `lang/tests/common/src/`, or
`lang/tests/interpret/src/`. Only add new files in `lang/tests/safety/`.

### Rule 2: Always Use `diagnoser` for Errors

Never use `std::cerr` or `std::cout` for error messages. Always use the compiler's
diagnostic system: `diagnoser.error(location, message)`.

### Rule 3: Feature Flags Default to OFF

Every new safety feature must have a code path for `SafeMode::Off` that does nothing.
This ensures existing code is unaffected.

### Rule 4: One Feature at a Time

Never implement multiple safety features in a single sub-step. Each feature is
independently testable and revertible.

### Rule 5: Build Before Testing

Always run `./scripts/build.sh --tcc` before running any tests. The compiler binary
must reflect the latest C++ changes.

### Rule 6: Use the Prompt Template

When asking an AI to implement a sub-step, use this template from the analysis doc:

```
I need to [DO SPECIFIC THING] in the Chemical compiler.

## Before state
In file [FILE] at line [LINE_NUMBER]:
[EXACT_EXISTING_CODE]

## After state
Change it to:
[EXACT_NEW_CODE]

## Files to modify
- [FILE_1] — [DESCRIPTION]
- [FILE_2] — [DESCRIPTION]

## Validation
[EXACT COMMANDS TO RUN]

## Constraints
- Do NOT modify any existing test files
- Default value must be OFF
- Use diagnoser, not std::cerr
```

### Rule 7: Document Every Decision

When a sub-step requires a design decision (e.g., "should this be an error or a
warning?"), document the decision in the safety test file as a comment. This gives
the next AI agent (or human) context for why things are the way they are.

---

## Feature-to-Phase Dependency Map

```
Phase 0 (Infrastructure)
├── PR1: Negative test runner
├── PR2: Module options parsing
├── PR3: SafeMode plumbing
└── PR4: release_safe mode

Phase 1 (Unsafe Boundaries) — depends on Phase 0
├── Step 1.1: Deref check
├── Step 1.2: Dealloc check
├── Step 1.3: Pointer arithmetic check
├── Step 1.4: Cast-to-ptr check
├── Step 1.5: @unsafe call check
├── Step 1.6: Extern call check
├── Step 1.7: Library opt-outs
└── Step 1.8: @unsafe("critical") annotation

Phase 2 (Module Configuration) — depends on Phase 1
├── Step 2.1: Read module safety options
├── Step 2.2: CLI override
├── Step 2.3: Default module safety level
├── Step 2.4: Opt out existing modules
└── Step 2.5: Verify opt-in works

Phase 3 (Runtime Checks) — depends on Phase 2
├── Step 3.1: Bounds checking
├── Step 3.2: Overflow checking
└── Step 3.3: Null pointer checking (optional)

Phase 4 (Container Safety) — depends on Phase 0
├── Step 4.1: Restrict vector.get()
├── Step 4.2: Where constraints on push()
├── Step 4.3: Where constraints on resize()
└── Step 4.4: InterfaceBits extension

Phase 5 (Lifetime Safety) — depends on Phase 1
├── Step 5.1: Reference scope tracking
├── Step 5.2: One-mutable-or-many-immutable (AI-hard)
└── Step 5.3: Reference storage in structs (AI-hard)

Phase 6 (Std Lib Wrappers) — depends on Phase 4
├── Step 6.1: Checked arithmetic
├── Step 6.2: NonNull<T>
└── Step 6.3: Safe vector API
```

### Critical Path

```
PR1 → PR3 → Step 1.1 → Step 1.5 → Step 1.7 → Step 2.1 → Step 2.4
```

This is the minimum viable safety system: negative test infrastructure,
safe mode plumbing, deref enforcement, @unsafe enforcement, library opt-outs,
module-level configuration, and existing code exemption.

### Parallel Workstreams

Phases 3, 4, and 5 are independent of each other and can be worked on in parallel
after Phase 2 is complete:

```
Phase 2 ──┬── Phase 3 (Runtime Checks)
           ├── Phase 4 (Container Safety)
           └── Phase 5 (Lifetime Safety)
```

Phase 6 depends on Phase 4 (needs the interface system).

---

## Summary: Total Effort Estimate

| Phase | Sub-steps | Estimated Lines Changed | AI Difficulty |
|-------|-----------|------------------------|---------------|
| Phase 0 | 4 steps | ~200 lines | Easy |
| Phase 1 | 8 steps | ~150 lines | Easy-Moderate |
| Phase 2 | 5 steps | ~100 lines | Easy |
| Phase 3 | 3 steps | ~100 lines | Moderate |
| Phase 4 | 4 steps | ~80 lines | Easy-Moderate |
| Phase 5 | 3 steps | ~60 lines | Hard |
| Phase 6 | 3 steps | ~100 lines | Easy |
| **Total** | **30 steps** | **~790 lines** | |

**All changes default to OFF.** Zero risk to existing code when flags are not used.
The full test suite is the safety net that catches any regressions.

---

*This plan is designed to be executed step-by-step by an AI agent, with each step
independently verifiable and revertible. The plan grows the safety system incrementally
without ever breaking the existing codebase.*

*Last updated: July 18, 2026.*
