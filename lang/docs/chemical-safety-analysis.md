# Chemical Language — Comprehensive Safety Analysis

> **Status: Research Document — Updated July 18, 2026**
>
> A thorough analysis of Chemical's current safety posture, missing safety features,
> implementation gaps, and **AI-friendly, low-effort additions**.
>
> **Key theme:** What can be implemented quickly and reliably by **AI code generators**,
> behind flags, with minimal performance impact, enabling **per-module opt-in**.
> All changes must preserve user convenience and not break existing code.
>
> **Critical insight:** Our test suite tests "things that should PASS" (positive tests).
> We have almost NO "things that should FAIL" tests (negative/compiler-diagnostic tests).
> This document defines a framework for both.

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [AI Suitability Ratings: How to Read This Document](#ai-suitability-ratings-how-to-read-this-document)
3. [Current Safety Features (Accurate)](#current-safety-features-accurate)
4. [Features That Already Exist](#features-that-already-exist)
5. [Testing Strategy: Two-Tier Framework](#testing-strategy-two-tier-framework)
6. [Per-Module Safety Configuration](#per-module-safety-configuration)
7. [User Convenience & Migration Path](#user-convenience--migration-path)
8. [AI Implementation Methodology](#ai-implementation-methodology)
9. [Feature Catalog](#feature-catalog)
   - [F1: Enable safe_context Enforcement](#f1-enable-safe_context-enforcement) **★ AI-ready**
   - [F2: Enable @unsafe Function Propagation](#f2-enable-unsafe-function-propagation) **★ AI-ready**
   - [F3: Extern C Call Safety Check](#f3-extern-c-call-safety-check) **★ AI-ready**
   - [F4: Std Library Checked Arithmetic](#f4-std-library-checked-arithmetic) **★ AI-ready**
   - [F5: More InterfaceBits & Auto-Derive](#f5-more-interfacebits--auto-derive) **★ AI-ready**
   - [F6: Container Constraint Enforcement](#f6-container-constraint-enforcement) **☆ AI-moderate**
   - [F7: Bounds Checking (--bounds-check)](#f7-bounds-checking---bounds-check) **☆ AI-moderate**
   - [F8: Nullable/NonNull Pointer Types](#f8-nullablenonnull-pointer-types) **☆ AI-moderate**
   - [F9: Overflow Checking (--overflow-check)](#f9-overflow-checking---overflow-check) **☆ AI-moderate**
   - [F10: Lifetime Escape Analysis (Basic)](#f10-lifetime-escape-analysis-basic) **☆ AI-hard**
   - [F11: Full --safe Flag Architecture](#f11-full---safe-flag-architecture) **★ AI-ready**
10. [Feature Flag Architecture Summary](#feature-flag-architecture-summary)
11. [Safety Infrastructure Reference](#safety-infrastructure-reference)
12. [Comparison with Other Languages](#comparison-with-other-languages)

---

## Executive Summary

Chemical is a systems programming language with deterministic destructors, move semantics,
and C-level performance. It occupies a design space between C and Rust.

**Key insight:** The compiler's safety infrastructure is surprisingly complete at the
**tracking** level — what's missing is **enforcement**. This makes many safety features
"finishing touches" rather than ground-up builds — perfect for AI implementation.

### The AI Opportunity

The best features for AI implementation share these properties:

| Property | Why It Matters for AI |
|----------|----------------------|
| **Small change surface** (≤20 lines) | AI can produce correct code when scope is narrow |
| **Existing pattern to follow** | AI can copy the `diagnoser.warn()` → `diagnoser.error()` pattern |
| **Clear "before/after" state** | AI can search for exact code and replace it |
| **Self-contained test** | AI can write a `.ch` file, compile it, and check exit code |
| **No dependency on other steps** | Each step is independently verifiable |
| **Feature flag defaults to OFF** | If AI produces incorrect code, existing tests still pass |

### The Testing Gap

Every existing test in `lang/tests/` is a **positive test** — code that should compile and
run correctly. We have almost zero **negative tests** — code that should FAIL to compile
with specific diagnostic messages. This is a critical gap because:

1. Safety features are about REJECTING bad code, not accepting good code
2. Without negative tests, a regression that makes the compiler ACCEPT unsafe code
   will NOT be caught
3. The "things that should fail" tests must be **shell-based integration tests**
   that invoke the compiler CLI and check stderr, not Chemical source tests
   (you can't write "this should fail to compile" in a language that needs to compile)

**Solution:** A separate test runner (`lang/tests/safety/run_safety_tests.sh`) that
invokes the compiler with `.ch` test files and checks:
- Exit code (0 for positive tests, non-zero for negative tests)
- Stderr content (must contain expected diagnostic message for negative tests)
- Stdout content (must contain "Test N [name] succeeded" for positive tests)

---

## AI Suitability Ratings: How to Read This Document

Each feature in the catalog gets an **AI-suitability rating**:

### ★ AI-ready

**What it means:** An AI can implement this feature in **1-2 prompts** with low risk.

**Properties:**
- ≤50 lines of C++/Chemical code change total
- Follows an existing pattern (copy-paste-modify)
- No new data structures or algorithms needed
- Testable by compiling a `.ch` file and checking exit code
- Feature flag defaults to OFF — zero risk of breaking existing code

**Examples:** Adding `diagnoser.error()` calls, defining new interfaces, adding flag plumbing.

### ☆ AI-moderate

**What it means:** An AI can implement this but needs **3-5 prompts** with intermediate
verification.

**Properties:**
- 50-200 lines of change across 3-5 files
- Requires understanding existing data flow (e.g., how GEP works in LLVM)
- May need new LLVM IR generation or codegen patterns
- Testable but requires more setup (runtime executable, not just compile check)

**Examples:** LLVM codegen changes, container constraint enforcement.

### △ AI-hard

**What it means:** Needs human guidance or multi-step verification. AI can assist
with sub-steps but should not implement the whole feature autonomously.

**Properties:**
- >200 lines across many files
- New analysis passes or data structures needed
- Subtle correctness requirements (e.g., soundness of lifetime analysis)
- Risk of false positives (rejecting valid code) or false negatives (accepting invalid code)

**Examples:** Borrow checker, full lifetime analysis, alias analysis.

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

## Testing Strategy: Two-Tier Framework

### The Core Problem

Currently, ALL tests in `lang/tests/` are **positive tests** — they verify that code
which SHOULD compile DOES compile and produces correct results.

**Missing:** Tests that verify code which SHOULD NOT compile is REJECTED with the
correct diagnostic message. Without these, safety features cannot be reliably tested.

### Proposed Solution: Shell-Based Negative Test Runner

Safety tests live in a **separate directory** (`lang/tests/safety/`) that is NOT part
of the main `build.lab` test suite. They are run by a shell script.

**Directory structure:**
```
lang/tests/safety/
├── run_safety_tests.sh        # The test runner (bash)
├── positive/                  # Code that SHOULD compile and run
│   ├── deref_in_unsafe.ch     # Should compile with --safe
│   ├── extern_in_unsafe.ch    # Should compile with --safe
│   ├── safe_by_default.ch     # Should compile WITHOUT --safe
│   └── expected_output/       # Expected stdout for positive tests
│       └── deref_in_unsafe.txt
├── negative/                  # Code that SHOULD FAIL compilation
│   ├── deref_in_safe.ch       # Should error with --safe: pointer deref
│   ├── extern_call_no_unsafe.ch  # Should error with --safe: extern call
│   ├── unsafe_fn_call.ch      # Should error with --safe: calling @unsafe fn
│   └── expected_errors/       # Expected stderr snippets (grep patterns)
│       ├── deref_in_safe.txt
│       ├── extern_call_no_unsafe.txt
│       └── unsafe_fn_call.txt
├── runtime/                   # Code that SHOULD trap at runtime
│   ├── oob_test.ch            # Should trap with --bounds-check
│   ├── overflow_test.ch       # Should trap with --overflow-check
│   └── expected_behavior/     # Description of expected runtime behavior
│       └── oob_test.txt       # "exits with SIGILL or calls abort()"
└── config/
    └── test_list.txt          # Master list: which tests to run with which flags
```

### How the Test Runner Works

```bash
#!/bin/bash
# lang/tests/safety/run_safety_tests.sh
# Runs safety tests without affecting the main test suite

set -e

COMPILER="${1:-cmake-build-debug/TCCCompiler}"
SAFETY_DIR="$(cd "$(dirname "$0")" && pwd)"
PASSED=0
FAILED=0

run_positive() {
    local test_file="$1"
    local flags="$2"
    local expected_output="$3"
    
    echo "=== POSITIVE: $test_file (flags: $flags) ==="
    local output
    output=$("$COMPILER" "$SAFETY_DIR/positive/$test_file" -o /dev/null \
        --mode debug_complete $flags 2>&1)
    local exit_code=$?
    
    if [ $exit_code -ne 0 ]; then
        echo "  FAIL: Expected compilation to succeed, got exit code $exit_code"
        echo "  Output: $output"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Optional: check stdout for expected test results
    if [ -n "$expected_output" ] && [ -f "$SAFETY_DIR/positive/expected_output/$expected_output" ]; then
        if ! echo "$output" | grep -q -f "$SAFETY_DIR/positive/expected_output/$expected_output"; then
            echo "  FAIL: Expected output not found"
            FAILED=$((FAILED + 1))
            return 1
        fi
    fi
    
    echo "  PASS"
    PASSED=$((PASSED + 1))
}

run_negative() {
    local test_file="$1"
    local flags="$2"
    local expected_error_pattern="$3"
    
    echo "=== NEGATIVE: $test_file (flags: $flags) ==="
    local output
    output=$("$COMPILER" "$SAFETY_DIR/negative/$test_file" -o /dev/null \
        --mode debug_complete $flags 2>&1 || true)
    local exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo "  FAIL: Expected compilation to FAIL, but it succeeded"
        echo "  Output: $output"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    # Check that the expected error message is in stderr
    if ! echo "$output" | grep -qi "$expected_error_pattern"; then
        echo "  FAIL: Expected error pattern '$expected_error_pattern' not found"
        echo "  Output: $output"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    echo "  PASS"
    PASSED=$((PASSED + 1))
}

run_runtime_trap() {
    local test_file="$1"
    local compiler_flags="$2"
    local link_flags="$3"
    
    echo "=== RUNTIME: $test_file (flags: $compiler_flags) ==="
    local exe="/tmp/safety_test_$$"
    
    "$COMPILER" "$SAFETY_DIR/runtime/$test_file" -o "$exe" \
        --mode debug_complete $compiler_flags $link_flags 2>&1 || {
        echo "  FAIL: Compilation failed (expected runtime test)"
        FAILED=$((FAILED + 1))
        return 1
    }
    
    # Run and expect non-zero exit (trap/signal)
    local exit_code=0
    "$exe" 2>&1 || exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo "  FAIL: Expected runtime trap but program exited normally"
        FAILED=$((FAILED + 1))
        return 1
    fi
    
    if [ $exit_code -ge 128 ]; then
        local signal=$((exit_code - 128))
        echo "  PASS: Program trapped with signal $signal"
    else
        echo "  PASS: Program exited with code $exit_code (expected failure)"
    fi
    PASSED=$((PASSED + 1))
    rm -f "$exe"
}

# --- Test Cases ---

# F1: safe_context enforcement
run_negative "deref_in_safe.ch" "--safe" "de-referencing.*not allowed"
run_positive "deref_in_unsafe.ch" "--safe" ""

# F2: @unsafe function propagation
run_negative "unsafe_fn_call.ch" "--safe" "unsafe function"

# F3: Extern C call check
run_negative "extern_call_no_unsafe.ch" "--safe" "extern.*unsafe"

# F7: Bounds checking runtime
run_runtime_trap "oob_test.ch" "--bounds-check" ""

# Regressions: existing tests must still pass
echo "=== REGRESSION CHECK ==="
cd "$SAFETY_DIR/../.."
./scripts/test.sh --tcc 2>&1 | tail -5
cd "$SAFETY_DIR"

echo ""
echo "========================"
echo "Results: $PASSED passed, $FAILED failed"
exit $FAILED
```

### How to Add a New Safety Test

**Positive test** (code that should compile):
```bash
# 1. Create the .ch file in positive/
cat > lang/tests/safety/positive/my_feature_ok.ch << 'EOF'
func main() {
    unsafe {
        var x = 42
        var p = &raw x
        return *p  // OK: in unsafe block
    }
}
EOF

# 2. Register in run_safety_tests.sh:
#    run_positive "my_feature_ok.ch" "--safe" ""
```

**Negative test** (code that should fail):
```bash
# 1. Create the .ch file in negative/
cat > lang/tests/safety/negative/my_feature_bad.ch << 'EOF'
func main() {
    var p = &raw x  # ERROR: taking address of undefined variable
    return *p       # Should error: deref in safe context
}
EOF

# 2. Create expected error pattern file
echo "de-referencing" > lang/tests/safety/negative/expected_errors/my_feature_bad.txt

# 3. Register in run_safety_tests.sh:
#    run_negative "my_feature_bad.ch" "--safe" "$(cat negative/expected_errors/my_feature_bad.txt)"
```

### How Negative Tests Validate AI Changes

For AI-implemented safety checks, the validation loop is:

```bash
# 1. Build the modified compiler
./scripts/build.sh --tcc

# 2. Run ONLY the negative tests (fast, ~2 seconds)
bash lang/tests/safety/run_safety_tests.sh cmake-build-debug/TCCCompiler 2>&1 | grep -E "PASS|FAIL"

# 3. Run the main test suite to check for regressions
./scripts/test.sh --tcc --no-build

# 4. If both pass, the change is correct
```

**This is the key insight:** AI changes can be validated in SECONDS by running
only the relevant negative tests, without waiting for the full test suite.

---

## Per-Module Safety Configuration

### The Goal

Allow modules to opt in to safety checks gradually, without affecting their dependencies
or dependents. A library compiled without `--safe` should work seamlessly with an
application compiled with `--safe`.

### Phase 1: Global Flags (Simplest — AI-ready)

For the first implementation phase, safety flags are **global command-line flags**:

```bash
# Compile everything with safety enforcement
cmake-build-debug/TCCCompiler my_app.mod -o my_app.exe --safe

# Compile with warnings only (good for migration)
cmake-build-debug/TCCCompiler my_app.mod -o my_app.exe --safe=warn

# Compile with bounds checking
cmake-build-debug/TCCCompiler my_app.mod -o my_app.exe --safe --bounds-check
```

**How it works:** The `--safe` flag is parsed in `CompilerMain.cpp` and stored in
`LabBuildCompilerOptions.safe_mode`. It's propagated to:
- `SymResLinkBody.safe_mode` — for symbol resolution phase checks
- `TypeVerify.safe_mode` — for type verification phase checks
- `CodegenEmitterOptions.safe_mode` — for LLVM codegen checks

**Files to modify (AI-ready, ~5 files, ~30 lines):**
1. `core/main/CompilerMain.cpp` — parse `--safe` flag
2. `compiler/lab/LabBuildCompilerOptions.h` — add `SafeMode safe_mode = SafeMode::Off`
3. `compiler/symres/SymResLinkBody.h` — add `SafeMode safe_mode` field
4. `compiler/typeverify/TypeVerify.h` — add `SafeMode safe_mode` field
5. `compiler/CodegenEmitterOptions.h` — add `SafeMode safe_mode` field

### Phase 2: Per-Module Flags (Extended — AI-moderate)

For the second phase, allow `chemical.mod` files to declare safety requirements:

```
// my_lib/chemical.mod
module my_lib

source "src"

import "std"

// Module-level safety flags
safe = true
bounds-check = true
overflow-check = true
```

**How it works:**

1. `ModuleFileData` gets new fields:
   ```cpp
   // In compiler/processor/ModuleFileData.h
   bool safe_mode_set = false;  // Was safe explicitly set?
   SafeMode safe_mode = SafeMode::Off;
   bool bounds_check = false;
   bool overflow_check = false;
   ```

2. The `.mod` file parser (`server/mod_file/Importer.cpp` or new parser) handles:
   ```
   safe = true     → safe_mode = SafeMode::Enforce
   safe = warn     → safe_mode = SafeMode::Warn
   bounds-check = true → bounds_check = true
   ```

3. `ModToLabConverter.cpp` generates `build.lab` code that calls new module APIs:
   ```chemical
   mod.set_safe_mode(SafeMode::Enforce)
   mod.set_bounds_check(true)
   ```

4. During compilation, `process_module_tcc()` reads these flags and passes them
   to the appropriate compilation phases.

**Validation:**

```bash
# Test that a module with safe=true rejects unsafe code
cat > /tmp/safe_mod/chemical.mod << 'EOF'
application safe_mod
source "src"
safe = true
EOF

cat > /tmp/safe_mod/src/main.ch << 'EOF'
func main() {
    var x = 42
    var p = &raw x
    return *p  // Should error: safe module
}
EOF

cmake-build-debug/TCCCompiler /tmp/safe_mod/chemical.mod -o /tmp/safe_mod.exe
# EXPECTED: compile error about dereference in safe context
```

### Phase 3: Mixed Safety Levels (Advanced)

Different modules can have different safety levels. A safe application can depend on
an unsafe library — the library's code is compiled without safety checks, but the
application's calls to the library go through `unsafe` boundaries:

```
// app/chemical.mod
application my_app
source "src"
import "../unsafe_lib"  // This lib is compiled without --safe
safe = true             // But this app has --safe
```

When calling functions from the unsafe library, the call site must be in an `unsafe {}`
block because the compiler can't guarantee the external library's safety.

**This is a significant design effort and is △ AI-hard.** Phase 1 and 2 are sufficient
for most use cases.

---

## User Convenience & Migration Path

### Design Principle: Safety Must Not Hurt Ergonomics

Users will reject safety features if they make common tasks painful. Every safety check
must be:

1. **Opt-in at the module level** — existing projects continue to work without changes
2. **Gradual** — users can fix violations one module at a time
3. **Transparent** — error messages must clearly explain what's wrong and how to fix it
4. **Pragmatic** — some operations are "safe enough" and shouldn't require `unsafe`

### Migration Workflow

**Step 1: Audit with warnings**
```bash
cmake-build-debug/TCCCompiler my_app.mod -o my_app.exe --safe=warn
```
All safety violations are reported as warnings. The project still compiles.
Users can see what needs to be fixed.

**Step 2: Fix violations**
```chemical
// Before:
func init_buffer() {
    var buf = malloc(1024)  // Warning: malloc requires unsafe
    // ...
}

// After:
func init_buffer() {
    unsafe {
        var buf = malloc(1024)  // OK: explicitly unsafe
    }
    // ...
}
```

**Step 3: Enable enforcement**
```bash
cmake-build-debug/TCCCompiler my_app.mod -o my_app.exe --safe
```
All violations are now errors. Any unfixed issues block compilation.

**Step 4: Lock it in (Phase 2+)**
```toml
# chemical.mod
safe = true
```
The module requires safety. CI will reject unsafe code.

### What Stays Convenient

| Operation | Safe Mode Behavior | Why It's Convenient |
|-----------|-------------------|---------------------|
| `arr[i]` on a static array | Always allowed (bounds-check is separate flag) | Array access is the most common operation |
| `a + b` on integers | Always allowed (overflow-check is separate flag) | Arithmetic is fundamental |
| `str.length()` / `vec.size()` | Always allowed | Property access has no safety implications |
| `*ptr` inside `unsafe { }` | Always allowed | Explicit unsafe block |
| Calling a regular function | Always allowed | Most functions are safe |
| `var x = y` (move semantics) | Already enforced | Existing behavior |
| Pattern matching | Always allowed | Control flow is safe |

### What Requires `unsafe` (in --safe mode)

| Operation | Why Unsafe |
|-----------|-----------|
| `*ptr` (pointer dereference) | Memory safety violation if pointer is invalid |
| `malloc()` / `free()` / `dealloc` | Manual memory management |
| `ptr + n` / `ptr - n` | Pointer arithmetic can overflow or go out of bounds |
| `as *mut T` casts (from non-pointer) | Creating a pointer from arbitrary data |
| Calling `@unsafe` functions | Explicit opt-in to unsafe behavior |
| Calling extern C functions | FFI bypasses all guarantees |

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
| **Never modify existing tests** | AI must only add new test files, never modify `.ch` files in `lang/tests/` |
| **Use only `diagnoser.error()` not raw `std::cerr`** | The compiler's diagnostic system must be used for user-facing errors |
| **Never remove `#include`** | AI may add includes but never remove them |
| **Always compile before testing** | `./scripts/build.sh --tcc` before running tests |
| **Feature flag defaults to OFF** | New safety features must default to disabled |
| **Compilation must not regress** | Run `./scripts/test.sh --tcc` before and after; same pass count |
| **One file per change** | Each sub-step should modify at most 2-3 files |

### Verification Checklist for Every Step

Before marking a step complete, the AI must:

- [ ] Build the compiler (`./scripts/build.sh --tcc`)
- [ ] Run the positive test (code that should compile → exit 0)
- [ ] Run the negative test (code that should NOT compile → exit non-zero + expected error in stderr)
- [ ] Run the full test suite to check for regressions (`./scripts/test.sh --tcc --no-build`)
- [ ] Verify that without any flag (--safe not passed), behavior is unchanged

### The AI Prompt Template

When asking an AI to implement a sub-step, use this template:

```
I need to [DO SPECIFIC THING] in the Chemical compiler.

## Before state
In file [FILE] at line [LINE_NUMBER]:
```cpp
[EXACT_EXISTING_CODE]
```

## After state
Change it to:
```cpp
[EXACT_NEW_CODE]
```

## Files to modify
- `[FILE_1]` — add/modify [DESCRIPTION]
- `[FILE_2]` — add/modify [DESCRIPTION]

## Validation
```bash
# Build
./scripts/build.sh --tcc

# Positive test: [DESCRIPTION — should succeed]
cmake-build-debug/TCCCompiler [TEST_FILE] -o /dev/null --mode debug_complete [FLAGS]
# Expected: exit 0

# Negative test: [DESCRIPTION — should fail]
cmake-build-debug/TCCCompiler [TEST_FILE] -o /dev/null --mode debug_complete [FLAGS]
# Expected: exit non-zero, stderr contains "[EXPECTED_ERROR_TEXT]"

# Regression check
./scripts/test.sh --tcc --no-build
```

## Constraints
- Do NOT modify any existing test files
- Default value must be OFF
- Use diagnoser, not std::cerr
```

---

## Feature Catalog

---

### F1: Enable safe_context Enforcement ★ AI-ready

**AI-suitability: ★ AI-ready** — 7 sub-steps, each ≤10 lines, follows existing patterns.

**Overview:** The `safe_context` flag is already tracked in `SymResLinkBody`. When set,
unsafe operations should produce errors (or at least warnings). Currently, only
`VisitDereferenceValue` produces a **warning**. We need to:
1. Promote the deref warning to error (behind `--safe` flag)
2. Add checks for other unsafe operations in `SymResLinkBody`

#### Sub-Step 1.1: Add --safe flag to LabBuildCompilerOptions

**AI scope:** ~5 lines in 1 file.

**Before:** `LabBuildCompilerOptions` has no safety flag.

**After:** Add `SafeMode` enum and `safe_mode` field.

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
```bash
./scripts/build.sh --tcc
# Should compile without errors
```

**AI constraints:**
- Default must be `Off` — no existing code breaks
- Use the same pattern as existing flags like `is_testing_env`

---

#### Sub-Step 1.2: Create SafeMode.h and wire SafeMode through

**AI scope:** ~15 lines across 3 files.

**Files to modify:**
- `compiler/SafeMode.h` — new file with the enum
- `compiler/symres/SymResLinkBody.h` — add `SafeMode safe_mode = SafeMode::Off` field
- `compiler/symres/SymResLinkBody.cpp` — update constructor call if needed
- `compiler/typeverify/TypeVerify.h` — add `SafeMode safe_mode = SafeMode::Off` field
- `compiler/typeverify/TypeVerify.cpp` — initialize from options

**Validation:**
```bash
./scripts/build.sh --tcc
# No behavior change yet
```

---

#### Sub-Step 1.3: Promote Deref Warning to Error with --safe

**AI scope:** ~10 lines in 1 file. **This is the simplest and most impactful single change.**

**Before:**
```cpp
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
    if(safe_context && safe_mode != SafeMode::Off) {
        switch(safe_mode) {
            case SafeMode::Enforce:
                diagnoser.error("de-referencing a pointer outside unsafe block is not allowed", value);
                break;
            case SafeMode::Warn:
                diagnoser.warn("de-referencing a pointer in safe context is prohibited", value);
                break;
            default: break;
        }
    }
    ...visit children...
}
```

**Validation:**
```bash
# Positive test: no flag → no diagnostic
cat > /tmp/deref_ok.ch << 'EOF'
func main() {
    var x = 42
    var p = &raw x
    return *p
}
EOF
cmake-build-debug/TCCCompiler /tmp/deref_ok.ch -o /dev/null --mode debug_complete
# EXPECTED: exit 0, no warning

# Negative test: --safe → error
cmake-build-debug/TCCCompiler /tmp/deref_ok.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: exit non-zero, stderr contains "de-referencing"

# Positive test: unsafe block → no error even with --safe
cat > /tmp/deref_unsafe.ch << 'EOF'
func main() {
    unsafe {
        var x = 42
        var p = &raw x
        return *p
    }
}
EOF
cmake-build-debug/TCCCompiler /tmp/deref_unsafe.ch -o /dev/null --mode debug_complete --safe
# EXPECTED: exit 0
```

**This is the single highest-impact change for the effort.** It's ~10 lines and
immediately makes `--safe` useful.

---

#### Sub-Step 1.4: Check DeallocStmt in safe_context

**AI scope:** ~5 lines in 1 file.

**Before:**
```cpp
void SymResLinkBody::VisitDeallocStmt(DeallocStmt* node) {
    visit(node->ptr);
}
```

**After:**
```cpp
void SymResLinkBody::VisitDeallocStmt(DeallocStmt* node) {
    if(safe_context && safe_mode != SafeMode::Off) {
        // Same switch pattern as Sub-Step 1.3
        check_safe("deallocating memory requires an unsafe block", node);
    }
    visit(node->ptr);
}
```

**Validation:**
```bash
cat > /tmp/dealloc_test.ch << 'EOF'
func main() {
    var p = malloc(100) as *mut int
    dealloc p  // Should error in --safe mode
}
EOF
cmake-build-debug/TCCCompiler /tmp/dealloc_test.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: error about dealloc requiring unsafe block
```

---

#### Sub-Step 1.5: Check Pointer Arithmetic in safe_context

**AI scope:** ~10 lines in 1 file.

**Before:** Pointer arithmetic (`ptr + n`, `ptr++`) is unrestricted.

**After:** In the expression visitor or arithmetic visitor, check if operands are pointers:

```cpp
// In SymResLinkBody, during expression visiting:
auto check_ptr_arith = [&](Value* val, BaseType* type) {
    if(safe_context && safe_mode != SafeMode::Off && type && type->is_pointer()) {
        check_safe("pointer arithmetic requires an unsafe block", val);
    }
};
// Call this when visiting binary ops involving pointers
```

**Validation:**
```bash
cat > /tmp/ptr_arith.ch << 'EOF'
func main() {
    var arr : [3]int = [1, 2, 3]
    var p = &raw arr[0]
    p = p + 1  // Should error in --safe mode
}
EOF
cmake-build-debug/TCCCompiler /tmp/ptr_arith.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: error about pointer arithmetic
```

---

#### Sub-Step 1.6: Check Cast to Ptr in safe_context

**AI scope:** ~5 lines in 1 file.

**After:** In `VisitCastedValue`, if the target type is a pointer, require unsafe:

```cpp
// In SymResLinkBody::VisitCastedValue:
if(safe_context && safe_mode != SafeMode::Off && target_type->is_pointer()) {
    check_safe("casting to pointer type requires an unsafe block", value);
}
```

---

### F2: Enable @unsafe Function Propagation ★ AI-ready

**AI-suitability: ★ AI-ready** — uncommenting and fixing existing code.

**Overview:** `FunctionDeclaration::is_unsafe()` exists. `@unsafe` annotation is parsed
and stored. But calling an `@unsafe` function from safe code is **not enforced**.
The check in `TypeVerify.cpp` is commented out.

#### Sub-Step 2.1: Uncomment the @unsafe Check in TypeVerify

**AI scope:** ~10 lines in 1 file.

**Before:**
```cpp
// TypeVerify.cpp line ~455
// if(func_decl->is_unsafe() && resolver.safe_context) {
//     ...error...
// }
```

**After:**
```cpp
if(func_decl->is_unsafe() && !verifier.is_unsafe && verifier.safe_mode != SafeMode::Off) {
    switch(verifier.safe_mode) {
        case SafeMode::Enforce:
            verifier.diagnoser.error("calling @unsafe function requires an unsafe block", call);
            break;
        case SafeMode::Warn:
            verifier.diagnoser.warn("calling @unsafe function from safe context", call);
            break;
        default: break;
    }
}
```

**Validation:**
```bash
cat > /tmp/unsafe_fn.ch << 'EOF'
@unsafe
func unsafe_thing() : int {
    return 42
}

func main() : int {
    return unsafe_thing()  // Should error in --safe mode
}
EOF
cmake-build-debug/TCCCompiler /tmp/unsafe_fn.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: error about calling @unsafe function

# Negative test: wrapping in unsafe block should fix it
cat > /tmp/unsafe_fn_ok.ch << 'EOF'
@unsafe
func unsafe_thing() : int {
    return 42
}

func main() : int {
    unsafe {
        return unsafe_thing()  // OK: in unsafe block
    }
}
EOF
cmake-build-debug/TCCCompiler /tmp/unsafe_fn_ok.ch -o /dev/null --mode debug_complete --safe
# EXPECTED: exit 0
```

**Files to modify:**
- `compiler/typeverify/TypeVerify.cpp` — uncomment and fix the check
- `compiler/typeverify/TypeVerify.h` — add `safe_mode` field if not done in F1

---

### F3: Extern C Call Safety Check ★ AI-ready

**AI-suitability: ★ AI-ready** — small helper + one visitor check.

#### Sub-Step 3.1: Add is_extern_fn() Helper

**AI scope:** ~3 lines in 1 file.

```cpp
// In ast/structures/FunctionDeclaration.h
inline bool is_extern_fn() const {
    return attrs.is_extern || attrs.is_dllimport;
}
```

**Validation:**
```bash
./scripts/build.sh --tcc
```

---

#### Sub-Step 3.2: Check Extern Calls in VisitFunctionCall

**AI scope:** ~10 lines in 1 file.

**After:** In `SymResLinkBody::VisitFunctionCall`, after resolving the callee:

```cpp
if(safe_context && safe_mode != SafeMode::Off) {
    // Get linked function declaration from the callee
    const auto callee = call->parent_val->get_chain_last_linked();
    if(callee && callee->kind() == ASTNodeKind::FunctionDecl) {
        const auto decl = callee->as_function_unsafe();
        if(decl->is_extern_fn()) {
            check_safe("calling extern C function requires an unsafe block", call);
        }
    }
}
```

**Validation:**
```bash
cat > /tmp/extern_call.ch << 'EOF'
func main() {
    printf("hello\n")  // extern C function → error in safe mode
}
EOF
cmake-build-debug/TCCCompiler /tmp/extern_call.ch -o /dev/null --mode debug_complete --safe 2>&1
# EXPECTED: error about extern C call
```

---

### F4: Std Library Checked Arithmetic ★ AI-ready

**AI-suitability: ★ AI-ready** — Chemical source code only (no C++ changes).

#### Sub-Step 4.1: Define Checked Arithmetic Interfaces

**AI scope:** ~15 lines in 1 Chemical source file.

**After:** Add to `lang/libs/core/ops.ch`:

```chemical
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
```bash
./scripts/build.sh --tcc
# Must compile without errors
```

---

#### Sub-Step 4.2: Implement CheckedAdd, CheckedSub, CheckedMul for Integer Types

**AI scope:** ~40 lines in 1 Chemical source file (repetitive, perfect for AI).

**After:** Implement in `lang/libs/std/src/ops_impl.ch` or a new file:

```chemical
impl CheckedAdd for i32 {
    func checked_add(&self, rhs : i32) : Option<i32> {
        var result = *self + rhs
        // In comptime: check for overflow
        // At runtime with --overflow-check: LLVM handles it
        // Basic implementation: just wrap (same as regular add)
        return Option::Some(result)
    }
}

impl CheckedSub for i32 {
    func checked_sub(&self, rhs : i32) : Option<i32> {
        var result = *self - rhs
        return Option::Some(result)
    }
}

// ... same for i8, i16, i64, u8, u16, u32, u64, int, uint, long, ulong
```

**Validation:**
```bash
cat > /tmp/checked_arith.ch << 'EOF'
func main() {
    var a : i32 = 100
    var b : i32 = 50
    match a.checked_add(b) {
        Option::Some(val) => {
            printf("checked_add: %d\n", val)
        }
        Option::None => {
            printf("checked_add overflow!\n")
        }
    }
}
EOF
cmake-build-debug/TCCCompiler /tmp/checked_arith.ch -o /dev/null --mode debug_complete
# EXPECTED: exit 0
```

**Note:** The `Option` return type is currently best-effort until LLVM with.overflow
intrinsics are wired up in codegen (see F9). The Chemical-level interface is still
valuable for API consistency.

---

### F5: More InterfaceBits & Auto-Derive ★ AI-ready

**AI-suitability: ★ AI-ready** — adding constants, defining interfaces, auto-derivation
logic follows existing patterns.

#### Sub-Step 5.1: Add New Bit Constants to InterfaceBits.h

**AI scope:** ~10 lines in 1 file.

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

---

#### Sub-Step 5.2: Define Corresponding Interfaces in Chemical Source

**AI scope:** ~5 lines in 1 Chemical source file.

```chemical
// lang/libs/core/interfaces.ch
public interface Destructible {}
public interface TriviallyRelocatable {}
public interface DefaultConstructible {}
```

---

#### Sub-Step 5.3: Register New Interfaces in CoreNodes

**AI scope:** ~5 lines in 1 file.

```cpp
// compiler/symres/CoreNodes.h
InterfaceDefinition* destructible_interface = nullptr;
InterfaceDefinition* default_constructible_interface = nullptr;
InterfaceDefinition* trivially_relocatable_interface = nullptr;
InterfaceDefinition* copy_interface = nullptr;  // already exists
```

---

#### Sub-Step 5.4: Auto-Set Destructible Bit During Struct Processing

**AI scope:** ~5 lines in 1 file.

**After:** In `SymResLinkBody::LinkMembersContainerNoScope` or similar:

```cpp
auto bits = container->interface_bits;
if(container->has_destructor()) {
    bits.set(InterfaceBits::DESTRUCTIBLE_BIT);
}
container->interface_bits = bits;
```

**Validation:**
```bash
cat > /tmp/auto_dtor_bit.ch << 'EOF'
struct HasDtor {
    var x : int
    @delete func delete(&mut self) {}
}

func <T> take_destructible(val : T) where T : Destructible {}

func main() {
    var h = HasDtor { x: 42 }
    take_destructible(h)  // Should compile: auto-Destructible
}
EOF
cmake-build-debug/TCCCompiler /tmp/auto_dtor_bit.ch -o /dev/null --mode debug_complete
# EXPECTED: exit 0
```

---

#### Sub-Step 5.5: Auto-Set DefaultConstructible Bit

**AI scope:** ~5 lines in 1 file.

```cpp
if(!container->has_constructor() && container->is_zeroable()) {
    bits.set(InterfaceBits::DEFAULT_CONSTRUCTIBLE_BIT);
}
```

---

#### Sub-Step 5.6: Auto-Set TriviallyRelocatable Bit

**AI scope:** ~10 lines in 1 file.

```cpp
auto is_trivially_relocatable = [](MembersContainer* c) -> bool {
    if(c->has_destructor()) return false;
    // Check all fields are trivially relocatable
    for(auto& member : c->members) {
        auto type = member->known_type();
        if(type && type->is_struct()) {
            auto inner = type->get_struct_def();
            if(inner && inner->has_destructor()) return false;
        }
    }
    return true;
};

if(is_trivially_relocatable(container)) {
    bits.set(InterfaceBits::TRIVIALLY_RELOCATABLE_BIT);
}
```

---

### F6: Container Constraint Enforcement ☆ AI-moderate

**AI-suitability: ☆ AI-moderate** — requires understanding the existing `Copy` interface
and where clause system. Chemical source changes only, no C++ changes.

#### Sub-Step 6.1: Add where T : Copy to vector::get()

**AI scope:** ~1 line change in 1 Chemical source file.

**Note:** This already exists! `func get(&self, index : size_t) : T where T : Copy`

#### Sub-Step 6.2: Add where constraints to vector::push(), vector::resize()

**AI scope:** ~5 lines across 1-2 Chemical source files.

**After:**
```chemical
// In lang/libs/std/src/vector.ch
func push(&mut self, value : T) where T : Copy {
    // current implementation uses memcpy internally
    memcpy(&raw mut data_ptr[self.size], &raw value, sizeof(T))
    intrinsics::forget(value)
}

func resize(&mut self, new_size : size_t) where T : DefaultConstructible {
    // Uses zeroed<T>() for new elements — requires DefaultConstructible
}
```

**Validation:**
```bash
cat > /tmp/vector_constraints.ch << 'EOF'
struct NoCopy {
    var x : int
    // NOT Copy — this struct should not be pushable
}

func main() {
    var v : std::vector<int>
    v.push(42)  // OK: int is Copy
    
    // var v2 : std::vector<NoCopy>
    // v2.push(NoCopy { x: 1 })  // Should error: NoCopy is not Copy
}
EOF
cmake-build-debug/TCCCompiler /tmp/vector_constraints.ch -o /dev/null --mode debug_complete
# EXPECTED: exit 0 (the commented-out test is not compiled)
```

---

### F7: Bounds Checking (--bounds-check) ☆ AI-moderate

**AI-suitability: ☆ AI-moderate** — requires understanding LLVM IR generation and GEP.
The test requires running a compiled executable, not just compiling.

#### Sub-Step 7.1: Add --bounds-check Flag

**AI scope:** ~3 lines across 2 files.

**After:**
```cpp
// compiler/CodegenEmitterOptions.h
bool bounds_check = false;
```

**Validation:**
```bash
./scripts/build.sh --tcc
```

---

#### Sub-Step 7.2: Pass Bounds Check Flag to LLVM Codegen

**AI scope:** ~30 lines in 1 file (LLVM.cpp).

**After:** In `IndexOperator::llvm_value()` and `IndexOperator::llvm_pointer()`,
insert bounds checks before GEP:

```cpp
if(gen.bounds_check && gen.out_mode != OutputMode::ReleaseFast) {
    auto idx = /* index value */;
    auto size = /* array size value */;
    auto in_bounds = gen.builder->CreateICmpULT(idx, size);
    
    auto trap_bb = llvm::BasicBlock::Create(*gen.context, "oob_trap", gen.current_func);
    auto cont_bb = llvm::BasicBlock::Create(*gen.context, "oob_cont", gen.current_func);
    
    gen.builder->CreateCondBr(in_bounds, cont_bb, trap_bb);
    
    gen.builder->SetInsertPoint(trap_bb);
    gen.builder->CreateCall(gen.intrinsic(llvm::Intrinsic::trap));
    gen.builder->CreateUnreachable();
    
    gen.builder->SetInsertPoint(cont_bb);
}
```

**Validation:**
```bash
cat > /tmp/oob_test.ch << 'EOF'
func main() {
    var arr : [3]int = [1, 2, 3]
    var val = arr[10]  // Out of bounds
}
EOF

# Without flag: UB (might crash, might not)
cmake-build-debug/Compiler /tmp/oob_test.ch -o /tmp/oob_test.exe --mode debug_complete
/tmp/oob_test.exe ; echo "exit: $?"
# EXPECTED: undefined behavior (might work, might crash)

# With flag: should trap
cmake-build-debug/Compiler /tmp/oob_test.ch -o /tmp/oob_test.exe --mode debug_complete --bounds-check
/tmp/oob_test.exe ; echo "exit: $?"
# EXPECTED: traps (exit code > 128 means signal)
```

**AI constraints:**
- Only add checks in non-release modes (check `gen.out_mode`)
- Use `@llvm.trap` intrinsic — don't use `assert()` or `abort()`
- Keep `inbounds = true` on GEP even with bounds check

---

### F8: Nullable/NonNull Pointer Types ☆ AI-moderate

**AI-suitability: ☆ AI-moderate** — Chemical source only, no C++ changes.
Requires understanding `Option<T>` pattern.

#### Sub-Step 8.1: Create NonNull<T> Wrapper in std

**AI scope:** ~30 lines in 1 Chemical source file.

```chemical
// lang/libs/std/src/ptr.ch (new file)
public struct NonNull<T> {
    var ptr : *mut T

    @constructor
    func make(ptr : *mut T) -> Option<NonNull<T>> {
        if(ptr != null) {
            return Option::Some(NonNull { ptr: ptr })
        }
        return Option::None()
    }

    func get(&self) -> &mut T {
        return &mut *ptr  // Safe: verified non-null at construction
    }

    func get_ptr(&self) -> *mut T {
        return ptr
    }
}
```

**Validation:**
```bash
cat > /tmp/nonnull_test.ch << 'EOF'
func main() {
    var x = 42
    match std::NonNull::make(&raw mut x) {
        Option::Some(ref nn) => {
            printf("value: %d\n", *nn.get())
        }
        Option::None => {
            printf("unexpected null\n")
        }
    }
}
EOF
cmake-build-debug/TCCCompiler /tmp/nonnull_test.ch -o /dev/null --mode debug_complete
# EXPECTED: exit 0
```

---

### F9: Overflow Checking (--overflow-check) ☆ AI-moderate

**AI-suitability: ☆ AI-moderate** — uses LLVM `*_with_overflow` intrinsics.
Requires understanding LLVM IR.

#### Sub-Step 9.1: Add --overflow-check Flag

**AI scope:** ~3 lines across 2 files.

```cpp
// compiler/CodegenEmitterOptions.h
bool overflow_check = false;
```

---

#### Sub-Step 9.2: Use LLVM WithOverflow Intrinsics for +, -, *

**AI scope:** ~40 lines in 1 file (LLVM.cpp).

**After:** In the LLVM backend's binary operation codegen:

```cpp
if(gen.overflow_check && is_arithmetic_operator(op) && is_integer_type(type)) {
    llvm::Intrinsic::ID intrinsic_id;
    switch(op) {
        case Operation::Addition:       intrinsic_id = llvm::Intrinsic::sadd_with_overflow; break;
        case Operation::Subtraction:    intrinsic_id = llvm::Intrinsic::ssub_with_overflow; break;
        case Operation::Multiplication: intrinsic_id = llvm::Intrinsic::smul_with_overflow; break;
        default: goto normal_path;
    }
    
    auto result = gen.builder->CreateBinaryIntrinsic(intrinsic_id, lhs, rhs);
    auto value = gen.builder->CreateExtractValue(result, 0);
    auto overflow = gen.builder->CreateExtractValue(result, 1);
    
    auto trap_bb = llvm::BasicBlock::Create(*gen.context, "overflow_trap", gen.current_func);
    auto cont_bb = llvm::BasicBlock::Create(*gen.context, "overflow_cont", gen.current_func);
    
    gen.builder->CreateCondBr(overflow, trap_bb, cont_bb);
    
    gen.builder->SetInsertPoint(trap_bb);
    gen.builder->CreateCall(gen.get_intrinsic(llvm::Intrinsic::trap));
    gen.builder->CreateUnreachable();
    
    gen.builder->SetInsertPoint(cont_bb);
    return value;
}
normal_path:
    // ... existing code ...
```

**Validation:**
```bash
cat > /tmp/overflow_test.ch << 'EOF'
func main() {
    var max_i32 = 2147483647
    var result = max_i32 + 1  // Should trap with --overflow-check
}
EOF

cmake-build-debug/Compiler /tmp/overflow_test.ch -o /tmp/overflow_test.exe \
    --mode debug_complete --overflow-check
/tmp/overflow_test.exe ; echo "exit: $?"
# EXPECTED: traps (exit code > 128)
```

---

### F10: Lifetime Escape Analysis (Basic) △ AI-hard

**AI-suitability: △ AI-hard** — requires new analysis pass, risk of false positives.
**Phase 1 (basic scope tracking) is AI-moderate**, Phase 2+ is AI-hard.

#### Sub-Step 10.1: Track Scope Depth (AI-moderate)

**AI scope:** ~10 lines in 1 file.

```cpp
// In SymResLinkBody.h
int scope_depth = 0;
```

Increment/decrement around block scope visiting.

---

#### Sub-Step 10.2: Track Reference Bindings (AI-moderate)

**AI scope:** ~15 lines in 2 files.

```cpp
struct RefBinding {
    VariableIdentifier* var;
    int scope_depth;  // depth at which the referent was created
};
std::vector<RefBinding> ref_bindings;
```

---

#### Sub-Step 10.3: Check Return of Reference to Local (△ AI-hard)

**AI scope:** ~20 lines in 1 file, but correctness is subtle.

```cpp
if(node->value && node->value->getType()->is_reference()) {
    for(auto& binding : ref_bindings) {
        if(binding.scope_depth >= scope_depth) {
            // Local variable being returned as reference
            if(safe_mode == SafeMode::Enforce) {
                diagnoser.error("returning reference to local variable", node);
            }
        }
    }
}
```

---

### F11: Full --safe Flag Architecture ★ AI-ready

**AI-suitability: ★ AI-ready** — this is the plumbing that makes all other features work.

#### Sub-Step 11.1: Define SafeMode Enum

```cpp
// compiler/SafeMode.h
#pragma once
#include <cstdint>

enum class SafeMode : uint8_t {
    Off = 0,
    Warn = 1,
    Enforce = 2,
};
```

---

#### Sub-Step 11.2: Wire --safe Through CLI

In `core/main/CompilerMain.cpp`, add to the CLI parsing:

```cpp
// In the prepare_options lambda:
auto& safe_opt = options.option_new("safe");
if(safe_opt.has_value()) {
    auto val = safe_opt.value();
    if(val == "warn") {
        opts->safe_mode = SafeMode::Warn;
    } else {
        opts->safe_mode = SafeMode::Enforce;
    }
} else if(options.has_value("safe")) {
    opts->safe_mode = SafeMode::Enforce;
}
```

**Validation:**
```bash
./scripts/build.sh --tcc
cmake-build-debug/TCCCompiler --help 2>&1 | grep -i safe
# Should show --safe flag description
```

---

## Feature Flag Architecture Summary

```
Flag                    | Scope          | Phase | Effect
------------------------|----------------|-------|-------------------------------------------
--safe                  | Global/Module  | 1     | All safety checks as errors
--safe=warn             | Global/Module  | 1     | All safety checks as warnings only
--bounds-check          | Global/Module  | 1     | Inserts array bounds checks (debug mode)
--overflow-check        | Global/Module  | 1     | Inserts overflow checks (debug mode)
```

**In `chemical.mod` (Phase 2):**
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
4. Per-module: add `safe = true` to `chemical.mod` for locked-down modules
5. Enable stricter checks one by one (`--bounds-check`, `--overflow-check`)

---

## Safety Infrastructure Reference

### Key Files by Feature

| Feature | Primary File | Secondary Files |
|---------|-------------|-----------------|
| safe_context enforcement | `SymResLinkBody.cpp` | `SymResLinkBody.h` |
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

### CLI Argument Pattern (Existing)

The existing CLI parsing in `CompilerMain.cpp` uses `CmdOptions` with a `CmdOption` array.
Each flag has a long name, optional short alias, and a type (`NoValue`, `SingleValue`,
`MultiValued`, `SubCommand`).

For new flags:
```cpp
// Declaration:
CmdOption("safe", CmdOptionType::SingleValue),  // --safe=warn or --safe
CmdOption("bounds-check", CmdOptionType::NoValue),  // --bounds-check

// Parsing:
auto& safe_opt = options.option_new("safe");
if(safe_opt.has_value()) { /* use safe_opt.value() */ }
else if(options.has_value("safe")) { /* flag was passed without value */ }
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
| **Flag-controlled safety** | **⚠️ Missing** | ❌ | **✅** | ❌ |

**Chemical's unique opportunity:** Moving from "tracked but not enforced" to
"tracked and enforced" with a flag is MUCH easier than building from scratch.
Most safety infrastructure exists — it just needs the "enforcement" layer.

---

## Implementation Priority Matrix

| Feature | AI Difficulty | Lines of Code | User Impact | Risk | Priority |
|---------|--------------|---------------|-------------|------|----------|
| F1.3 Deref → Error with --safe | Very Easy | 10 | High | Very Low | **P0** |
| F11 --safe flag plumbing | Very Easy | 20 | High | Low | **P0** |
| F3 Extern C checks | Very Easy | 15 | Medium | Very Low | **P1** |
| F2 @unsafe enforcement | Easy | 10 | Medium | Low | **P1** |
| F5 InterfaceBits extension | Easy | 30 | Medium | Low | **P2** |
| F4 Checked arithmetic (lib) | Easy | 50 | Medium | Low | **P2** |
| F6 Container constraints | Easy | 10 | Medium | Low | **P2** |
| F7 Bounds checking (LLVM) | Moderate | 40 | High | Medium | **P3** |
| F9 Overflow checking (LLVM) | Moderate | 50 | High | Medium | **P3** |
| F8 NonNull<T> wrapper | Moderate | 30 | Low | Low | **P3** |
| F10 Lifetime analysis | Hard | 100+ | High | High | **P4** |

**Immediate next steps (P0):**
1. Create `compiler/SafeMode.h` — 1 file, 5 lines
2. Add `safe_mode` to `LabBuildCompilerOptions.h` — 1 file, 3 lines
3. Parse `--safe` in `CompilerMain.cpp` — 1 file, 10 lines
4. Pass `safe_mode` to `SymResLinkBody` — 2 files, 5 lines
5. Promote deref warning to error — 1 file, 10 lines
6. Create `lang/tests/safety/run_safety_tests.sh` — 1 file, 80 lines
7. Create first negative test — 1 file, 10 lines

**Total for P0: ~7 files, ~120 lines, ~2 hours of AI time.**

---

*Document updated from comprehensive codebase analysis.*
*Last updated: July 18, 2026.*
