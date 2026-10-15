---
name: MIR (C backend)
description: The Chemical compiler's MIR (mid-level IR) lowering path — architecture, current status (C and LLVM suites green, legacy 2c deleted), the AST→MIR lowering and per-backend emission pipeline, the coroutine/async lowering, type-spelling rules, destructor/move semantics via drop flags, and the hard-won codegen gotchas. Also carries the post-migration evaluation (measured compile-time/memory/generated-code/binary-size regressions) and the consolidation+performance action plan needed for the JVM/JS backends. Load before touching compiler/mir/*, compiler/CodegenMir.cpp, compiler/async/*, ASTProcessor MIR integration, or debugging generated C.
---

# MIR — the compiler's C lowering path

The authoritative living document is **`compiler/mir/README.md`**. Read it
first; it has the architecture, the full list of what is lowered, the exact
type-spelling rules, the known issues, and the hard-won gotchas. This skill is a
pointer plus the most important operational facts.

> **⚠️ EVALUATION + ACTION PLAN (read first if your task is perf, binary size,
> or a new backend):** `lang/docs/mir-migration-evaluation.md`. The migration is
> green and functionally complete, but it currently regresses compile time
> (~2.3×), peak memory (~+40%), generated C (~+70%), executable size (~+60%), and
> the compiler binary (~+7.5%), and the type/declaration layer is **still
> AST-based** (`MIRDeclEmitter`) with no aggregate layout in MIR. The plan's
> actions A–E explain how to reach "one small MIR core + thin C/LLVM/JVM/JS
> backends". Do not add to the AST-based declaration path.
>
> **MIR → LLVM:** the LLVM target now emits from MIR and the main `--llvm` suite
> is green; the handoff/contract is `lang/docs/mir-llvm-migration.md`. The old AST
> LLVM lowering is dead but not yet deleted.

## 🚫 Binding rule — no backend may read the AST; every backend works from MIR

The AST is a **front-end** artifact (parser, symres, type-checking, `MIRLowerer`).
**No code generator may touch it.** Both backends (`MIREmitter` for C,
`compiler/CodegenMir.cpp` for LLVM) and every future one (JVM, JS, WASM) are
**dispatchers over MIR only**:

| A backend needs | It reads | Never |
|---|---|---|
| types, layout, field offsets | `MIRTypeTable`, `MIRLayout`, `MIRTypeRecord`/`MIRFieldInfo` | `BaseType::size()`, `sizeof`/`offsetof` of a source type |
| calling convention, sret/param rules | `MIRABI` | AST function/parameter types |
| per-function facts (block offsets, fall-throughs, place writes/addressed, param aliasing) | `MIRFacts` (`compute_function_facts`) | re-running the analysis in the emitter |
| names, mangling, linkage | MIR symbol table / `MIRFunction::name` | `mangler_`/AST declaration walks |
| constants and initializers | `MIRConstantPool`, `MIRConstExprTable` (`MIRConstExpr.h`; global initializers) | AST `Value*` literals |
| aggregate/prototype/global declarations | MIR declaration tables (`MIRDeclPrinter` + `MIRGlobal` now; `MIRTypeDecl`/`MIRPrototype` as they land) | `StructDefinition`/`FunctionDeclaration` member walks |

Concretely, a backend must **not**: include an `ast/` header; take an
`ASTNode*`/`BaseType*`/`StructDefinition*`/`FunctionDeclaration*` parameter; walk
source nodes; or re-derive layout, ABI, declarations, destructor rules or
lifetime from source form. The **only** AST-aware MIR components are the lowering
boundary (`MIRTypeBuilder`, `MIRLowerer`) and the front-end declaration
collection pass — they *feed* MIR and are never a backend's source of truth.

When a backend needs a fact MIR does not publish, **publish it in MIR** (a field
or type fact on the record, a declaration record, a fact pass) — never reach back
into the AST. This is what makes JVM/JS/WASM thin printers instead of new front
ends, and it is the rule that keeps C and LLVM from drifting.

Full statements: `lang/docs/mir-implementation-plan.md` §0 rule 9,
`lang/docs/mir-design.md` §16, `compiler/mir/README.md` (top + *Invariants*).

## Status

- Branch `mir`. Contract: `lang/docs/mir-implementation-plan.md`,
  `lang/docs/mir-design.md`. Migration contract: `lang/docs/mir-llvm-migration.md`.
  Post-migration evaluation + plan: `lang/docs/mir-migration-evaluation.md`.
- **MIR is the only C path; the legacy 2c visitor is deleted.** All TCC suites are
  green:
  `./scripts/test.sh --tcc` → 2300 passed (0 failed); `--interpret` → 1890;
  `--libs` → 718/719 (flaky atomic CAS); `--plugins` → 1296; `--async` → 53;
  `--process` → 127; `--server` → 8/9 (flaky HTTP double-free).
  `MIRTests` green (1049 checks).
- **Measured regressions (2026-10-12, L1, `Debug`, `debug_quick`, vs pre-MIR
  `main`):** main-suite cold compile 2.80 s → 6.49 s (2.3×); peak RSS 43.4 →
  60.4 MB (+40%); probe C 8,285 → 14,115 lines (+70%); probe exe 341 → 543 KB
  (+59%); `tests-tcc.exe` 3.24 → 5.32 MB (+64%); `TCCCompiler` debug binary
  65.1 → 70.0 MB (+7.5%). Details and root causes: the evaluation doc §1–§2.
- The old per-function legacy bridge no longer exists — a lowering failure is a
  diagnostic, not a fallback. Do **not** add fallbacks.
- **The LLVM target now emits from MIR and the *main* suite is green:**
  `./scripts/test.sh --llvm` → 2298 / 2298. The old AST lowering is still present
  (undeleted) and emits nothing. The other LLVM suites are *not* green yet —
  `--llvm --libs` aborts inside the **compiler** while building the lib test exe
  (no test binary produced). See the migration doc §2.3 before trusting a
  red `--llvm <suite>` run as a codegen result.

## Files

- `compiler/mir/MIRLowerer.{h,cpp}` — AST→MIR lowering (includes AST headers),
  including `lower_async_function` / `async_lower_poll_body` / `async_lower_ramp`.
- `compiler/mir/MIRTypeBuilder.{h,cpp}` — `BaseType*` → `TypeId` (only core file
  that includes AST types). `named_struct(name)` creates an opaque named struct
  (used for the coroutine frame).
- `compiler/mir/MIRBuilder.{h,cpp}` — typed construction API
  (`alloca_frame`, `async_frame_alloc`, `call_indirect_sret`, …).
- `compiler/mir/MIREmitter.{h,cpp}` — MIR→C (AST-free).
- `compiler/mir/MIRDeclEmitter.{h,cpp}` — AST-based C declarations /
  prototypes / vtables (the collection/decl layer that is *not* yet fully MIR
  driven; see the evaluation doc actions A/B). Global **definitions** are now
  MIR-driven (`implement_global` → `MIRDeclPrinter::const_expr_c` over a
  `MIRConstExpr` tree); the AST value walk is gone.
- `compiler/mir/MIRVtable.{h,cpp}` — the **shared** interface-vtable layout
  (`MIRVtableSlot`, `mir_vtable_slots`, `mir_vtable_slot_type`): slot order and
  self-erased slot types, consumed by both the C declaration emitter and the
  LLVM emitter so the layout exists once. AST-aware (a slot is an interface
  method), so it is in the compiler but not in MIRTests.
- `compiler/CodegenMir.cpp` — MIR→LLVM lowering (AST-free, `COMPILER_BUILD`
  only); the driver is `ASTProcessor::mir_compile_module` in
  `compiler/ASTCompiler.cpp`.
- `compiler/mir/MIRTypes.h` (`MIRStorageClass`), `MIRInstruction.h` (opcodes),
  `MIRFunction.h` (`MIRPlaceDef.frame_field`), `MIRTypeTable.h`, `MIRArena.h`,
  `MIRDump.h`, `MIRVerifier.h`, `MIRConstantPool.h`, `MIRConstExpr.h`
  (global-initializer trees; `MIRGlobal` lives in `MIRModule.h`),
  `MIRSymbolTable.h`.
- `compiler/async/AsyncCTypes.{h,cpp}` — resolve the `FutureHandle<T>` /
  `FutureTable<T>` / `Poll<T>` / `Context*` types; **shared** by MIR's C and
  LLVM async lowering so both agree.
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
19. **An empty aggregate is zero-sized.** Never pad it with an `i8` in the LLVM
    type lowering: C spells `struct S { }` (size 0) and the interpreter's
    `byte_size` also says 0, so padding made `sizeof` disagree and shifted every
    field of a struct embedding an empty aggregate.
20. **An aggregate `Alloca` must be zeroed** — the C emitter emits a zero loop
    for every struct/union/variant/array `Alloca` and the lowerer relies on it
    (a partial initializer only copies the literal bytes). The LLVM `Alloca`
    case must emit the same `memset` at the same program point.
21. **A field name is a dotted path** (`Base.Mid.Leaf.field` for an inherited
    member, `Case.param` for a variant payload). Any resolver must walk every
    segment, not just the first dot.
22. **`dyn<T>(param)` must use the argument pointer**, not the parameter's
    by-value copy (kept in `MIRLowerer::aggregate_param_ptrs_`). The copy is a
    callee local, so a fat pointer to it dangles on return.

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
