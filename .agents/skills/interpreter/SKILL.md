---
name: Interpreter Internals
description: Comprehensive guide to Chemical interpretation and comptime evaluation after the MIR migration — the MIR interpreter engine, the comptime driver, the residual AST expression folder, and how to debug `--interpret` and comptime failures.
---

# Interpreter Internals

There is **no AST statement interpreter** any more. `compiler/Interpreter/` is
gone. Interpretation runs the *same* MIR the code generators consume, and
lowering-time comptime calls run on the same engine. Read
`lang/docs/mir-interpretation-plan.md` first — it records the design, the phases,
and the measured state.

## What runs where

| Path | Engine | Entry point |
|------|--------|-------------|
| `--arg-interpret` (interpretation job) | `mir::MIRInterpreter` | `do_interpretation_job()` in `compiler/lab/LabBuildCompiler.cpp` |
| `comptime func` call during lowering | `mir::MIRInterpreter` via `mir::MIRComptime` | `MIRComptime::evaluate` / `evaluate_block` / `evaluate_value` |
| symres-time `comptime if` condition, array length, `const` initializer | AST **expression** folder | `Value::evaluated_value` + compiler-intrinsic `FunctionDeclaration::call` overrides |

The first two are the real interpreters. The third is a residual, deliberately
kept: `symres` decides which branch/texture even exists before MIR is built, so
it must fold an expression — typically a call to a compiler intrinsic, whose body
is a C++ `call` override. See §5.4 of the plan for why this floor exists.

## Interpretation job (`--arg-interpret`)

`do_interpretation_job()`:

1. Parses, symres's and typechecks every module of the job.
2. Lowers each module's function bodies and globals into **one shared MIR
   program** (`ASTProcessor::interpret_module`, collect mode).
3. Finds `main()`, resolves its mangled name with the same `Mangler` the lowerer
   used, and runs it with `mir::MIRInterpreter::run`.
4. Emits no object code and links no executable.

`interpretation_mode` is **false** during the job on purpose, so
`comptime if(intrinsics::is_interpretation())` takes the compiled branch and the
test suite prints through `printf`. Interpretation is defined to have the same
semantics as the compiled binary — the engine differs, the language does not.

## The MIR interpreter (`compiler/mir/MIRInterpreter.{h,cpp}`)

- AST-free and single-threaded: it reads only MIR tables.
- `prepare()` resolves everything the hot loop needs (type facts, field offsets,
  symbol targets, constant bits, global storage) once, into flat dense side
  tables indexed by MIR ids.
- Public API: `run(entry, argc, argv, out_result, error)`,
  `call(name, args, nargs, out_result, error)` (scalar result),
  `call_sret(name, args, nargs, out_buf, error)` (aggregate result),
  `has_function(name)`.
- Unsupported MIR (e.g. coroutine/`Async*` opcodes, `DynamicCall` vtables)
  reports through `error` rather than executing; see §10 of the plan.

## The comptime driver (`compiler/mir/MIRComptime.{h,cpp}`)

Lowering-time `comptime func` calls are lowered into their own MIR module (with
every function they reach, transitively) and executed; the raw result is bridged
back to an AST `Value*` that the lowering keeps folding. It **declines** anything
the engine does not reproduce identically and the caller then falls back to the
AST expression folder, so every suite stays green. Covered today: generics,
flat-struct (sret) results, no-body intrinsics the lowerer folds, pointer
arithmetic/comparison, and comptime-mode intrinsics
(`is_comptime`/`is_runtime`/`is_interpretation`).

## The residual AST expression folder

Only *expressions* are folded, never statements:

- `Value::evaluated_value` implementations (`IntNumValue`, `BoolValue`,
  `StringValue`, `CastedValue`, `SizeOfValue`, `IsValue`, `AccessChain`,
  `VariableIdentifier`, `StructValue`, `ArrayValue`, …).
- `InterpretScope` (`ast/base/InterpretScope.h`) is just a small value map for
  parameter binding (`FunctionDeclaration::call` binds params and folds a body
  that is a single `return <expr>`), plus arithmetic folding in
  `InterpretScope::evaluate`.
- `PointerValue` simulates pointers with `behind`/`ahead` bounds for comptime
  pointer arithmetic.
- Compiler intrinsics in `ast/utils/GlobalFunctions.cpp` override
  `FunctionDeclaration::call`; this is where `intrinsics::*` values come from.

Do **not** add statement execution back here, and do not add a new AST
interpreter: a capability belongs in MIR (opcode + engine support), so every
backend and the engine keep agreeing.

## Test suite entry points

| Mode | Entry Function | Module | File |
|------|---------------|--------|------|
| Interpret | `main()` | `interpret_tests` | `lang/tests/interpret/src/main.ch` |
| Native compiled | `main()` | `main` | `lang/tests/src/tests.ch` |
| Common (shared) | `run_common_tests()` | `common_tests` | `lang/tests/common/src/main.ch` |
| Native-common | `run_native_common_tests()` | `native_common_tests` | `lang/tests/native_common/src/main.ch` |

Because the interpret suite runs the same MIR, a test that passes compiled and
fails interpreted is an **engine** bug (`MIRInterpreter` diverging from the
emitter), not a language limitation. Write the failure down as such.

## Debugging

1. `./scripts/test.sh --tcc --interpret` — the interpretation suite.
2. `./scripts/test.sh --tcc --interpret -bt` — run under GDB with a backtrace.
3. `./cmake-build-debug/MIRTests` — engine smoke tests; extend these when you
   add an opcode.
4. `[lab] error: MIR interpreter: <message>` is the engine declining or trapping;
   the message names the function and opcode.
5. Lowering-time comptime failures surface as ordinary diagnostics at the call
   site (the driver reports why it declined, the AST path reports why it could
   not fold).
6. `MIRDump` (`MIRDump.{h,cpp}`) prints the MIR a module lowered to — the fastest
   way to see what the engine will actually execute.

## Adding coverage

1. Pick the right location: `lang/tests/src/comptime/` for comptime features,
   `lang/tests/common/src/` for tests shared by both modes,
   `lang/tests/native_common/src/` for pointer arithmetic.
2. Write the function and `test("name", () => { ... })` the assertion.
3. Register it (`run_common_tests()` / `run_native_common_tests()` /
   `main()`), or annotate it `@test` for the compiled runner.
4. Run both `--tcc` and `--tcc --interpret`; they must agree.
