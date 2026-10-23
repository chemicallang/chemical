---
name: Safety Implementation
description: How the Chemical safety system is implemented and tested — the full-parity plan, stable SAFE-* diagnostics, report→migrate→enforce workflow, safety test suites, and current implementation status. Load before implementing or reviewing any safety rule, borrow-checker change, or unsafe-boundary check.
---

# Chemical Safety Implementation

This skill is the working guide for the full safety parity program. The canonical
plan is [`lang/docs/safety-implementation-plan.md`](../../../lang/docs/safety-implementation-plan.md).
The source tree wins over any status note here.

## Current status (2026-10-23)

**Milestone M0 / S0.1–S0.6 are implemented.**

Implemented:

- `Diag` has an optional stable `code` (`core/diag/Diagnostic.h`).
- Coded diagnostics render as `error[SAFE-*]:` / `warn[SAFE-*]:`; uncoded
  diagnostics keep the old `error:` rendering.
- `Diag::with_code(...)` and `Diag::format`/`Diag::ansi` include the code and
  related-information notes.
- `ASTDiagnoser`/`Diagnoser` can attach codes through
  `empty_diagnostic(..., code)` and `with_code`.
- First real codes:
  - `SAFE-INIT-001` — use of an uninitialized variable.
  - `SAFE-UNSAFE-001` — dereference of a raw pointer in safe context
    (report-only warning today).
- Negative harness accepts both `error:` and `error[SAFE-` and has
  `expect_compile_warning_id`.
- Safety suite at `lang/tests/safety/` (`--safety`), wired through
  `lang/tests/build.lab` and `scripts/test.sh`.
- Runtime-failure harness (`expect_runtime_panic`) using `process::execute` and
  compiler subprocesses; smoke + panic tests pass on TCC and LLVM.
- `--safety-report` read-only audit (S0.2): text table and JSONL
  (`--safety-report json`), collected after type checking and before codegen.
  Categories include raw pointers, unsafe blocks, FFI, panics, allocators,
  memory calls, inline asm, and mutable globals. Per-module dedup. Generic
  master impl bodies are skipped for now; concrete declarations are reported.
- Tests: `lang/tests/negative/src/safety_diagnostics.ch`; safety suite now has
  smoke, report, and runtime-panic tests (`3/3` on TCC and LLVM).

- Module/CLI safety opt-outs removed (S0.4): `safety`, `safe_mode`, and
  `checks.*` no longer exist; `option safety = false` is rejected. Module-file
  `option` parsing now consumes its leading identifier.
- `unsafe(expr)`, `da_in_unsafe`, and `@allow_zeroed` are still kept until their
  replacement stages; they are not user-facing opt-outs.
- S0.5 MIR/interpreter hardening (partial): O(log n) source interning, hard
  errors for unknown interpreter opcodes, `AtomicFence` no-op for single-threaded
  comptime, and `--mirtests` (`4524 checks`). Interpreter provenance/bounds/UAF
  checks are deferred to M5.7.

- S0.6 `--check` runs MIR lowering + structural verification without emission
  or linking; a good module exits 0 with no executable, an uninitialized read
  exits 1 with `SAFE-INIT-001` and no executable.

Next:

- M1 type-system foundations: lifetimes, opt-in `Copy`/`Drop`, slices,
  Box/Vec/String, traits, HRTB, dyn, UTF-8, full C ABI prerequisites.
- Parity corpus/manifest is M8.1 (not an M0 blocker).
- Baseline/performance gate and `scripts/safety-audit.sh` (S0.3).
- Remove module/CLI opt-outs (S0.4).

## Non-negotiable rules

1. **No user opt-out.** No `SafeMode::Off`, module flag, CLI escape, or
   per-dependency exemption.
2. **No error-suppressing allowlist.** A rule is report-only until the whole
   in-repo tree is migrated, then enforced. Report-only is a debug development
   mode and must be compiled out of release builds.
3. **`unsafe` is explicit.** It permits only inherently unchecked operations;
   it never disables ownership, initialization, or borrow checking.
4. **Never accept a pattern the checker cannot prove safe.** Emit a specific
   `SAFE-*` diagnostic and a rewrite suggestion.
5. **Every safety rule has a negative test, a positive test, and (where
   applicable) a runtime-failure test.**
6. **No diagnostic mentions the reference language.**
7. **Every commit is green.** Use the path→suite gate in the plan (§12.7).

## Report → migrate → enforce

Every safety rule follows the same three-step workflow:

1. **Report-only**
   - Add the diagnostic with a stable `SAFE-*` code.
   - Emit it as a warning; behavior is unchanged.
   - Add a negative/positive test and update the audit report.
2. **Migrate**
   - Fix every in-repo occurrence (libraries, tests, `build.lab`, CBI).
   - Report hits must reach zero.
   - Do not add an allowlist.
3. **Enforce**
   - Flip the diagnostic to an error.
   - Include the migration diff and a CI assertion of zero report hits.
   - Add the release-gate negative test.

Runtime rules (bounds, overflow, ABI, drop order) use
**observe/count → migrate → enable** because they have no warning form.

## Adding a coded diagnostic

```cpp
// Simple: code attached at the diagnoser
diagnoser.error(loc).with_code("SAFE-INIT-001") << "message";

// Warning (report-only)
diagnoser.warn(node).with_code("SAFE-UNSAFE-001") << "message";

// If you already hold a Diag&
auto& d = diagnoser.error(node);
d.with_code("SAFE-BORROW-002");
d << "cannot borrow ...";
```

Codes are stable and grouped by domain:

| Prefix | Domain |
|---|---|
| `SAFE-OWN-*` | moves, drops, partial moves |
| `SAFE-BORROW-*` | aliasing / loans |
| `SAFE-LIFE-*` | lifetimes / escapes |
| `SAFE-INIT-*` | initialization / definite assignment |
| `SAFE-VALID-*` | invalid values / transmute / packed |
| `SAFE-BOUNDS-*` | indexing |
| `SAFE-OVERFLOW-*` | arithmetic |
| `SAFE-SEND-*` | Send/Sync / races |
| `SAFE-UNSAFE-*` | unsafe boundary |
| `SAFE-FFI-*` | FFI / C ABI |
| `SAFE-ASYNC-*` | async/Pin |
| `SAFE-DEP-*` | future remote dependency pass |

Do not reuse a code for a different rule. Do not renumber existing codes.

## Test harness

Compile-fail tests live in `lang/tests/negative/src/`:

```chemical
@test
func neg_example(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "... unsafe source ..."
    expect_compile_error(env, "example", ch, "SAFE-OWN-001")
}
```

Helpers in `lang/tests/negative/src/main.ch`:

- `expect_compile_success(env, name, ch)`
- `expect_compile_error(env, name, ch, expected_substring)`
- `expect_compile_error_with_mod(env, name, ch, expected_substring, mod_content)`
- `expect_compile_warning_id(env, name, ch, expected_id)` — compiles and checks
  `warn[SAFE-*]`.
- `expect_compile_output_contains(...)`
- `expect_emitted_symbol(...)`

Coded errors render `error[SAFE-*]:`; the harness accepts both coded and
uncoded errors. Prefer asserting the **code**, not the full message.

Runtime/parity tests live in `lang/tests/safety/` and run with `--safety`.
Compile-fail tests stay in `--negative` so the default suites remain fast.

## Commands

```bash
# Fast compile-fail gate
./scripts/test.sh --tcc --negative --no-build

# Safety suite (smoke, report, runtime-panic)
./scripts/test.sh --tcc --safety --no-build

# Read-only audit
./cmake-build-debug/TCCCompiler <module> --no-cache --safety-report
./cmake-build-debug/TCCCompiler <module> --no-cache --safety-report json

# Lower + verify without emitting/linking
./cmake-build-debug/TCCCompiler <module> --check --no-cache

# Audit baseline gate
./scripts/safety-audit.sh            # fail on growth
./scripts/safety-audit.sh --update   # record a reviewed baseline

# Compiler-size / audit-time baseline
./scripts/safety-baseline.sh
./scripts/safety-baseline.sh --update

# Main TCC regression
./scripts/test.sh --tcc --no-build

# Full relevant gate for safety/compiler changes
./scripts/test.sh --tcc
./scripts/test.sh --llvm
./scripts/test.sh --tcc --interpret
./scripts/test.sh --tcc --safety
./scripts/test.sh --tcc --negative
```

When compiler C++ changed, omit `--no-build` or rebuild first:

```bash
./scripts/build.sh --tcc -j 16
./scripts/build.sh --llvm -j 16
```

## Where safety work goes

| Area | Files |
|---|---|
| Diagnostic codes/formatting | `core/diag/Diagnostic.*`, `core/diag/Diagnoser.h`, `compiler/ASTDiagnoser.*` |
| Unsafe context/checks | `compiler/symres/SymResLinkBody.*`, `compiler/typeverify/TypeVerify.*` |
| Lifetimes/type facts | `compiler/safety/*` (new), `ast/types/*`, `parser/*` |
| MIR move/borrow/validity | `compiler/mir/MIRMoveCheck.*`, `MIRBorrowCheck.*`, `MIRValidity.*` (new) |
| Runtime checks | `compiler/mir/MIRRuntimeChecks.*`, C/LLVM emitters |
| Standard library | `lang/libs/core`, `lang/libs/std`, `lang/libs/cstd` |
| Tests | `lang/tests/negative/src/safety_*.ch`, `lang/tests/safety/` |
| Plan | `lang/docs/safety-implementation-plan.md` |
| Audit metrics | `lang/docs/safety/migration-metrics.md` |

## Current architecture reminders

- Backends never read the AST; all safety facts belong in MIR.
- MIR types are lifetime-erased; regions live in per-function side tables.
- `MIRMoveCheck` is the sole producer of init/drop state; elaboration inserts
  flags from its plan.
- Every emitted/interpreted `MIRFunction` (including lambdas, async poll/drop,
  stubs, comptime) must go through `lower_and_check`.
- Bounds lower as `Compare` + `CondBr` + panic call, not a mid-block opcode.
- C signed overflow/float→int must be lowered explicitly for TCC and LLVM.

## Review checklist for a safety change

- [ ] Stable `SAFE-*` code added, not reused.
- [ ] Report-only mode added first (unless runtime-only).
- [ ] Negative test asserts the code.
- [ ] Positive test compiles/runs.
- [ ] Unsafe-context counterpart behaves correctly.
- [ ] Libraries/tests migrated; zero report hits for the rule.
- [ ] TCC + LLVM + interpreter as applicable.
- [ ] `--safety-report` / `scripts/safety-audit.sh` updated.
- [ ] Plan status updated with what landed and what remains.
- [ ] No allowlist, no opt-out, no TODO/stub.
