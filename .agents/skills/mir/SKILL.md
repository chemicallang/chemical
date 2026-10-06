---
name: MIR (C backend)
description: The Chemical compiler's MIR (mid-level IR) C lowering path — architecture, current status (all suites green, including --libs/--plugins/--async/--process/--server), the AST→MIR lowering and MIR→C emission pipeline, the coroutine/async lowering, type-spelling rules that must match legacy prototypes, destructor/move semantics via drop flags, and the hard-won codegen gotchas. Load before touching compiler/mir/*, compiler/async/*, ASTProcessor MIR integration, or debugging generated C.
---

# MIR — the compiler's C lowering path

The authoritative living document is **`compiler/mir/README.md`**. Read it
first; it has the architecture, the full list of what is lowered, the exact
type-spelling rules, the known issues, and the hard-won gotchas. This skill is a
pointer plus the most important operational facts.

## Status

- Branch `mir`. Contract: `lang/docs/mir-implementation-plan.md`,
  `lang/docs/mir-design.md`.
- **All suites are green:**
  `./scripts/test.sh --tcc` → 2263 passed;
  `--interpret` → 1875; `--libs` → 719; `--plugins` → 1296;
  `--async` → 53; `--process` → 127; `--server` → 9.
  `MIRTests` green (1049 checks).
- MIR owns the bodies of top-level `FunctionDeclaration`s **and async
  functions** (the coroutine lowering is implemented in MIR, not the legacy C
  visitor). Declarations and non-function top-level nodes still go through the
  legacy visitor.
- A **per-function legacy bridge** still exists for constructs MIR does not yet
  lower: when MIR lowering fails, `mir_translate_after_declaration` prints
  `[MIR] lowering failed (<fn>): <reason> -- falling back to the legacy visitor`
  and lets the legacy visitor emit that function. **The goal is to eliminate
  this entirely — do NOT add new fallbacks.** The remaining fallbacks are
  unrelated general gaps (chained method calls, aggregate construction,
  indexing aggregate elements, kind 8/16/29/48 values), not the async/server
  paths.

## Files

- `compiler/mir/MIRLowerer.{h,cpp}` — AST→MIR lowering (includes AST headers),
  including `lower_async_function` / `async_lower_poll_body` / `async_lower_ramp`.
- `compiler/mir/MIRTypeBuilder.{h,cpp}` — `BaseType*` → `TypeId` (only core file
  that includes AST types). `named_struct(name)` creates an opaque named struct
  (used for the coroutine frame).
- `compiler/mir/MIRBuilder.{h,cpp}` — typed construction API
  (`alloca_frame`, `async_frame_alloc`, `call_indirect_sret`, …).
- `compiler/mir/MIREmitter.{h,cpp}` — MIR→C (AST-free).
- `compiler/mir/MIRTypes.h` (`MIRStorageClass`), `MIRInstruction.h` (opcodes),
  `MIRFunction.h` (`MIRPlaceDef.frame_field`), `MIRTypeTable.h`, `MIRArena.h`,
  `MIRDump.h`, `MIRVerifier.h`.
- `compiler/async/AsyncCTypes.{h,cpp}` — resolve the `FutureHandle<T>` /
  `FutureTable<T>` / `Poll<T>` / `Context*` types; **shared** by the legacy 2c
  path and MIR so both agree.
- `compiler/async/AsyncLoweringPlan.h`, `AwaitNormalizePass.{h,cpp}` — the shared
  coroutine plan (`build_async_plan`) and the await-hoisting normalizer.
- `compiler/ASTProcessor.cpp` — `mir_translate_after_declaration` + callbacks
  (`set_mangler`, `set_comptime_if_resolver`, `set_comptime_eval`,
  `set_comptime_ctor_eval`, `set_symbol_lookup`). Async functions emit
  `async_pre_decls()`, then `async_poll()`, then `async_post_decls()`, then the
  ramp.
- `compiler/mir/tests/mir_tests.cpp` — standalone `MIRTests` (core only).

## Build / test

```bash
bash -lc "source scripts/msvc_env.sh && cmake --build cmake-build-debug --config Debug --target TCCCompiler MIRTests -j 8"
./cmake-build-debug/MIRTests.exe

# ALWAYS clear the test-exe cache after changing the compiler, or you run stale C:
Remove-Item -Recurse -Force lang/tests/build/chemical-tests.dir, lang/tests/build/lab
./scripts/test.sh --tcc

# keep the translated C for a suite's test executable (e.g. --libs, --async):
./cmake-build-debug/TCCCompiler.exe lang/tests/build.lab -o lang/tests/build/tests-tcc.exe \
    --mode debug_quick --no-cache --arg-test-libs --emit-c
# -> lang/tests/build/chemical-lib-tests.dir/Translated.c
```

The `--arg-test-*` flag selects the suite: `--arg-test-libs`, `--arg-test-plugins`,
`--arg-test-async`, `--arg-test-process`, `--arg-test-server`. The per-module C
lives in `lang/tests/build/<module>.dir/Translated.c` (e.g. `universal.dir`,
`css.dir`, `md.dir`).

## Async / coroutine lowering (MIR)

An `async func f() : T` has its return type rewritten by symres to
`FutureHandle<T>`. MIR lowers it (there is no legacy emission any more):

- **Helpers** (C text): the frame struct `struct f__frame { uint32_t __state; T
  __result; Context* __cx; <child_i>…; <slot_id>…; <drop_flag_id>…; }`, the
  `f__poll` prototype, the `f__drop` definition, and the
  `f__vtbl` definition (`FutureTable<T>` with `.poll`/`.drop`).
- **`__poll`** (MIR): `switch(af->__state)` dispatch; state `0` → the body, state
  `i+1` → resume site `i`, default → `Ready(af->__result)`. Frame-resident
  params are bound to frame fields; spilled params are loaded from the frame at
  entry.
- **`__drop`**: destroys the live destructible slots at the suspended state and
  drops the live child future, then `chemical_async_frame_free`.
- **Ramp**: `AsyncFrameAlloc`, `state = 0`, `cx = 0`, store params into frame
  slots, then `AsyncRampFinish` returns the handle. For the **eager** path (no
  await sites) the ramp runs the body inline, redirecting `return e` into
  `af->__result` and branching to the done block before `AsyncRampFinish`.

New opcodes (`compiler/mir/MIRInstruction.h`):

- `AsyncFrameAlloc` — allocate the frame (`chemical_async_frame_alloc`).
- `AsyncAwait` — poll the child future; on `Pending` set the resume state (first
  pass only), spill the live locals, write `Pending` and `return`; on `Ready`
  take `__chx__p.Ready.value`, drop the child and null its frame.
- `AsyncFinish` — `state = DONE; *ret = Ready(af->__result); return;`.
- `AsyncRampFinish` — set the final state and return the `FutureHandle<T>`.

Frame-resident storage: `MIRStorageClass::FrameField` (a field
`__chx__af->__chx_slot_N`, declared by the frame struct, no `Alloca`) and
`FramePtr` (the frame pointer local, spelled `__chx__af`). `place_name` maps
these; `MIRPlaceDef.frame_field` holds the field-name ConstantId.

Key correctness details:

- **Continuation block**: after an `AsyncAwait`, create a fresh block, `br` to
  it and record it (`async_cont_blocks_`). The resume block must branch there,
  not to the poll's default — otherwise a resumed coroutine re-runs the first
  await or completes early.
- **The child is consumed**: after storing the awaited operand into the child
  frame field, `mark_moved` the source place so its destructor does not drop the
  same frame twice.
- **`af->__cx`** must be stored at the top of `__poll` so child polls can reach
  the waker.
- Frame-resident destructibles use the frame drop flag `__chx_drop_N` (created by
  `register_destructible` when the place is a `FrameField`), so completion and
  cancellation agree on liveness; `mark_moved` clears it.

## Critical gotchas (hard-won, one per fixed bug)

1. **The test-exe C is cached per module.** After a compiler change, clear
   `lang/tests/build/chemical-tests.dir` (and `lang/tests/build/lab`). Use
   `--emit-c` to inspect.
2. **`value id 0 == MIR_NULL`.** Do not use `result_or_place == MIR_NULL` as
   "no result" for value-producing opcodes. A `MIROperand::constant(id, …)` takes
   a **ConstantId**, not a value id — reading `.bits` requires the ConstantId.
3. **Aggregate params are pointer-typed places.** `&param` is the pointer for an
   aggregate param, but the slot address for a genuine reference param. Decide
   with `needs_aggregate_path(ast_type)`. Drop/Destroy and field/indirect stores
   must deref pointer-typed places.
4. **Type spelling must match the legacy prototypes** emitted by
   `declare_module` (char/int/unsigned, `_Bool`, `const` pointers/references,
   `__chemical_fat_pointer__` for dynamic, function-pointer/array declarators).
   A mismatch is a TinyCC "incompatible types for redefinition" error.
5. **Function-pointer types returning an aggregate use the legacy sret spelling**
   `void(*)(struct T*, …)`, not `struct T(*)(…)`; `CallIndirect` passes the sret
   place first (`call_indirect_sret`).
6. **`&raw` / `&mut` on a reference yields the referent**, not the parameter slot
   (`lower_address_of` loads a `Reference`-typed place). **`&raw obj.field`**
   (address of a struct field) walks an `AccessChain` and returns the last field
   address. **`&raw arr[i]`** returns the element address (`index_addr` /
   `index_addr_ptr`, spelled with `__typeof__`).
7. **Post-increment/decrement yields the OLD value** (`lower_incdec_value`), pre
   yields the new one. A wrong result corrupts encoders (`buf[bi++] = hex[...]`).
8. **Compound assignment on index/deref targets** (`arr[i] += x`, `*p += z`) must
   load/apply/store, not store the RHS.
9. **Blocks/`Scope`/`UnsafeBlock` are scopes**: lower their statements with
   `lower_scope_nodes` so locals are destroyed at block end.
10. **Default arguments** go through `lower_arg_converted` with the parameter
    type (implicit constructors run).
11. **Static method references** (`Type::method`) and qualified module constants
    (`std::NPOS`) must resolve the leaf, not address the type.
12. **String literals are `char*`** (indexable). **Global arrays decay** to a
    pointer to the first element (`arr[i]`, not `(&arr)[i]`).
13. **Anonymous structs/unions** (a field type with no spellable C name) are
    addressed with `__typeof__(base.field)* v = &base.field;`.
14. **Chain walker intermediate pointer fields** (`a.b.c` where `b` is a pointer)
    must deref (`a.b->c`).
15. **`memset` conflicts with cstd's `extern void* memset(...)`** — emit an inline
    zero loop instead. Zero-length arrays cannot take `= {0}`.
16. **Enum members** use `get_default_index()`, not 0. **Namespace-qualified
    calls returning aggregates** are not constructors; resolve the callee first.
17. **Comptime**: evaluate via the interpreter, call `evaluated_value` on a
    returned `%runtime_value` while the scope is alive, resolve
    `CapturedComptimeVariable` bridge nodes and captured call refs.
18. **Scope destruction**: `lower_scope`/`lower_scope_nodes` drops the
    destructibles it created; `return` drops all outstanding; `lower_function`
    drops the rest. Moves clear a drop flag (`mark_moved`).

## Process/server test portability (not language bugs)

- `/tmp` is not a valid Win32 `CreateProcess` working directory, nor a valid
  `fs` path — tests use `comptime if(def.windows)` to pick `C:/` (process) or the
  `TEMP` env dir (server fixture).
- On Windows the accepted socket inherits the listener's non-blocking mode, so
  the http/server `accept_main` and `serve_coro` call `net::set_blocking(s)`
  before the blocking request/body reader runs.

## Known open issues

- Per-function legacy bridge for lambdas / expressive strings / runtime blocks /
  chained method calls / aggregate construction / indexing aggregate elements.
- Aggregate layout (size/fields) not yet filled in `MIRTypeRecord`.
- `MIRTests` must stay green before every MIR commit.
