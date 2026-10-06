---
name: MIR (C backend)
description: The Chemical compiler's MIR (mid-level IR) C lowering path — architecture, current status (main suite green), the AST→MIR lowering and MIR→C emission pipeline, type-spelling rules that must match legacy prototypes, destructor/move semantics via drop flags, and the known intermittent JIT crash. Load before touching compiler/mir/*, ASTProcessor MIR integration, or debugging generated C.
---

# MIR — the compiler's C lowering path

The authoritative living document is **`compiler/mir/README.md`**. Read it
first; it has the architecture, the full list of what is lowered, the exact
type-spelling rules, the known issues, and the hard-won gotchas. This skill is a
pointer plus the most important operational facts.

## Status

- Branch `mir`. Contract: `lang/docs/mir-implementation-plan.md`,
  `lang/docs/mir-design.md`.
- **Main test suite is green: `./scripts/test.sh --tcc` -> 2234 passed, 0
  failed.** `MIRTests` green (1049 checks).
- MIR owns the bodies of top-level `FunctionDeclaration`s; declarations and
  non-function top-level nodes still go through the legacy visitor.
- A **per-function legacy bridge** exists: when MIR lowering fails,
  `mir_translate_after_declaration` prints
  `[MIR] lowering failed (<fn>): <reason> -- falling back to the legacy visitor`
  and lets the legacy visitor emit that function. This is temporary; the goal is
  to remove it. `mir_eligible_function` also routes a few shapes to legacy.

## Files

- `compiler/mir/MIRLowerer.{h,cpp}` — AST→MIR lowering (includes AST headers).
- `compiler/mir/MIRTypeBuilder.{h,cpp}` — `BaseType*` → `TypeId` (only core file
  that includes AST types).
- `compiler/mir/MIRBuilder.{h,cpp}` — typed construction API.
- `compiler/mir/MIREmitter.{h,cpp}` — MIR→C (AST-free).
- `compiler/mir/MIRTypes.h`, `MIRInstruction.h`, `MIRFunction.h`,
  `MIRTypeTable.h`, `MIRArena.h`, `MIRDump.h`, `MIRVerifier.h`.
- `compiler/ASTProcessor.cpp` — `mir_translate_after_declaration` + callbacks
  (`set_mangler`, `set_comptime_if_resolver`, `set_comptime_eval`,
  `set_comptime_ctor_eval`, `set_symbol_lookup`).
- `compiler/mir/tests/mir_tests.cpp` — standalone `MIRTests` (core only).

## Build / test

```bash
bash -lc "source scripts/msvc_env.sh && cmake --build cmake-build-debug --config Debug --target TCCCompiler MIRTests -j 8"
./cmake-build-debug/MIRTests.exe

# ALWAYS clear the test-exe cache after changing the compiler, or you run stale C:
Remove-Item -Recurse -Force lang/tests/build/chemical-tests.dir, lang/tests/build/lab
./scripts/test.sh --tcc

# keep the translated C for the test executable:
./cmake-build-debug/TCCCompiler.exe lang/tests/build.lab -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache --emit-c
# -> lang/tests/build/chemical-tests.dir/Translated.c
```

## Critical gotchas

1. **The test-exe C is cached per module.** After a compiler change, clear
   `lang/tests/build/chemical-tests.dir` (and `lang/tests/build/lab`). Otherwise
   `Translated.c` and the exe are stale. Use `--emit-c` to inspect.
2. **`value id 0 == MIR_NULL`.** Do not use `result_or_place == MIR_NULL` as
   "no result" for value-producing opcodes.
3. **Aggregate params are pointer-typed places.** `&param` is the pointer for an
   aggregate param, but the slot address for a genuine reference param. Decide
   with `needs_aggregate_path(ast_type)`. Drop/Destroy and field/indirect stores
   must deref pointer-typed places.
4. **Type spelling must match the legacy prototypes** emitted by
   `declare_module` (char/int/unsigned, `_Bool`, `const` pointers/references,
   `__chemical_fat_pointer__` for dynamic, function-pointer/array declarators).
   A mismatch is a TinyCC "incompatible types for redefinition" error.
5. **`memset` conflicts with cstd's `extern void* memset(...)`** — emit an inline
   zero loop instead. Zero-length arrays cannot take `= {0}`.
6. **Enum members** use `get_default_index()`, not 0.
7. **Namespace-qualified calls returning aggregates** are not constructors;
   resolve the callee first.
8. **Comptime**: evaluate via the interpreter, call `evaluated_value` on a
   returned `%runtime_value` while the scope is alive, resolve
   `CapturedComptimeVariable` bridge nodes and captured call refs.
9. **Compound assignment on struct fields** must load/apply/store.
10. **Scope destruction**: `lower_scope` drops the destructibles it created;
    `return` drops all outstanding; `lower_function` drops the rest. Moves clear
    a drop flag (`mark_moved`).

## Known open issues

- **Intermittent JIT crash** (not fixed): the build script's JIT calls an
  uninitialized function pointer (`0xcccccccc00000002`), ~30-40% of
  `TCCCompiler lang/tests/build.lab ...` runs, no test exe produced. Not caused
  by MIR lowering (reproduces with destructors disabled). See the README for the
  investigation notes and next steps.
- Per-function legacy bridge for lambdas / expressive strings / runtime blocks /
  a few unresolved imported build-script bodies.
- Aggregate layout (size/fields) not yet filled in `MIRTypeRecord`.
