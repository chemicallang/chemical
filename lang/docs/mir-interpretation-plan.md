# MIR Interpretation Plan (v1.0)

Status: **Phases 0–4 DONE** (interpretation job runs on MIR; all suites green).
Phase 5 (comptime on MIR, then delete the AST interpreter) is **remaining**.
Constraint (binding): **maximum performance, both at compile time and at
run time.** The MIR interpreter must be fast enough that
`./scripts/test.sh --tcc --interpret` stays "really fast", and the code we add
must be written for speed from the first line — no "make it work, optimize
later" on the hot path.
Owner: MIR core (`compiler/mir/`), the build job driver
(`compiler/lab/LabBuildCompiler.cpp`), and the front end
(`compiler/ASTProcessor.cpp`).

> **Status (this session).** `--arg-interpret` now lowers every module to MIR
> (`ASTProcessor::interpret_module` + a job-wide `mir::MIRInterpProgram`) and
> runs it through the new `mir::MIRInterpreter` engine
> (`compiler/mir/MIRInterpreter.{h,cpp}`). `interpretation_mode` is off, so the
> tests take the compiled (`printf`) branches: interpretation is the same
> semantics as C/LLVM, exactly the goal below.
>
> Green: `MIRTests` 4400 · `--tcc` 2307/2307 · `--llvm` 2305/2305 ·
> `--tcc --interpret` **1896/1896 (via MIR)** · `--libs` 718/719 (known env
> failure) · `--async` 53/53.
>
> Phase 5 is the remaining work: the `comptime` evaluation callbacks
> (`MIRLowerer::set_comptime_eval` / `_value_eval` / `_ctor_eval` / `_block_eval`
> / `set_comptime_if_resolver`) still call the AST interpreter
> (`CTranslationContext::eval_comptime_call`, `Value::evaluated_value`), and
> `compiler/Interpreter/Core.cpp` is still built. Migrating those to "lower the
> comptime callee to MIR and run it" is what lets the AST interpreter be
> deleted; it is a separate, larger change and is **not** part of this session.

---

## 0. Goal (one sentence)

Move the **interpretation job** (`--arg-interpret`, `do_interpretation_job`) off
the AST interpreter and onto **a MIR execution engine**, then use that same
engine for comptime value evaluation, and finally **delete the AST
interpreter** (`compiler/Interpreter/Core.cpp` and the `Value::evaluated_value`
machinery it depends on).

The end state: **every execution path in the compiler — C, LLVM, and
interpretation — consumes MIR.** There is exactly one lowering (AST → MIR) and
one set of semantics; interpretation is only a different *execution engine* for
the same MIR, not a second implementation of the language.

---

## 1. Why

1. **Semantic unification.** Today the AST interpreter is a second, hand-written
   model of the language (documented quirks: `InterpretVector`,
   pointer-bounds enforcement, "uninitialized" storage-class gotchas, move
   semantics re-implemented with pointer matching, `&raw struct_val` unsupported,
   …). It drifts from the backends by construction. Interpreting MIR removes the
   second model: comptime, interpretation, C and LLVM all agree because they
   consume the same IR.
2. **Test-value.** `--arg-interpret` currently exercises the *AST interpreter*,
   never the lowering. Running MIR means the interpretation suite exercises the
   **same lowering as `--tcc`**, so lowering bugs surface in a fast suite instead
   of only in generated C/LLVM.
3. **One backend pattern.** A MIR interpreter is the same shape as a new code
   generator: a dispatcher over `MIROpcode`. It is the cheapest possible seed for
   future bytecode/JVM/JS execution and keeps every backend thin.
4. **Retire surface area.** Deleting the AST interpreter removes a large,
   correctness-sensitive subsystem (move-tracking, temp destruction, scope
   destruction) that the compiler must otherwise keep in sync forever.

### 1.1 Non-goals (explicitly out of scope)

- Not an async executor. `await` remains eager during interpretation (design
  §11 of `async-await-design.md`); async coroutines are not driven.
- Not a debugger. Source locations in MIR exist for codegen debug info; the
  interpreter uses them only for diagnostics.
- Not changing compiled semantics. Generated C and LLVM must be bit-identical
  before/after every phase.

---

## 2. Binding rules

1. **MIR only.** No new code may call the AST interpreter for a runtime
   evaluation. The interpretation job lowers to MIR and runs MIR.
2. **No feature flags, no fallback.** No `--interp-mir` flag, no "if the MIR
   engine can't run this function, use the AST interpreter". A function the
   engine cannot execute is a failure to be fixed, never routed around.
3. **No AST in the engine.** The engine includes no `ast/` header, takes no
   `ASTNode*`, and reads only MIR tables. (Same rule as every backend;
   `mir-implementation-plan.md` §0 rule 9.)
4. **Generated C is frozen.** Every phase leaves `--tcc` and `--llvm` output
   unchanged; the invariance harness below is the gate.
5. **Performance is a correctness property.** The runtime acceptance criteria in
   §6 are gates, not aspirations. A phase that makes `--interpret` more than
   ~10 % slower than the AST interpreter does not land until it is fixed.

---

## 3. Current state (verified in the tree)

### 3.1 The interpretation job

`LabBuildCompiler::do_interpretation_job` (`compiler/lab/LabBuildCompiler.cpp:4873`):

1. flatten deps; for each module: `import_module_files_direct`, `sym_res_module`,
   `type_verify_module_parallel`; **never lowers to MIR, never emits C**;
2. find `main`;
3. interpret module-level `VarInitStmt`s on the `GlobalInterpretScope`;
4. `main_fn->call(...)` — AST interpretation (`ast/structures/FunctionDecl.cpp`).

The job sets `interpretation_mode = true`
(`GlobalInterpretScope(..., true)`), so:

- `intrinsics::is_comptime()` → `true` (`ast/utils/GlobalFunctions.cpp:744`),
- `intrinsics::is_interpretation()` → `true` (`GlobalFunctions.cpp:516`),
- therefore `comptime if` in `lang/tests/common/src/test.ch` selects the
  **`intrinsics::expr_println`** branch (an intrinsic `FunctionDeclaration` whose
  C++ `call()` writes to `std::cout`).

### 3.2 The AST interpreter (to be deleted)

- `compiler/Interpreter/Core.cpp` (statement interpretation),
- `Value::evaluated_value` overrides across `ast/values/*`,
- `InterpretScope` / `GlobalInterpretScope` (`ast/base/*`),
- `FunctionDeclaration::call` (`ast/structures/FunctionDecl.cpp`),
- interpreter-friendly std types in `ast/utils/GlobalFunctions.cpp`
  (`InterpretVector`, …).

### 3.3 What MIR already gives us

- `compiler/mir/MIRLowerer.*` lowers functions to `MIRFunction` (opcodes, places,
  values, blocks, cleanup scopes) from the resolved AST.
- The **exact** runtime ABI is in `compiler/mir/MIRABI.h`:
  - struct-like/array **returns** use a hidden **sret** pointer,
  - struct-like/array **params** are passed **by pointer**, except a typedef
    struct (`TF_TYPEDEF`, the `dyn<I>` fat pointer) passed by value,
  - pointers and references share the C spelling.
- Layout is in MIR: `MIRTypeRecord.size/alignment`, `MIRFieldInfo.offset`
  (`compiler/mir/MIRLayout.h`). No `sizeof`/`offsetof` needs the AST.
- Global initializers are an AST-free tree: `MIRGlobal` +
  `MIRConstExpr` (`compiler/mir/MIRConstExpr.h`).
- `ASTProcessor::implement_module` already has a **collect mode**
  (`mir_llvm_collect = true`) that lowers every function body into
  `MIRModule::functions` and does **not** reset the lowering arena
  (`MIRTaskContext::begin_function(false)` → `compiler/ASTProcessor.cpp:1901`).
  The LLVM backend already rides this path.

### 3.4 Opcode surface the C emitter produces

From `emit_function_c` (`compiler/mir/MIREmitter.cpp:593`): `ConstInt/Bool/
Float/Double/String/Null`, `Alloca`, `Param`, `Load`, `Store`, `FieldAddr`,
`IndexAddr`, `AddressOf`, `Unary`, `Binary`, `Compare`, `Cast`, `Call`,
`Return`, `CopyInit`, `MoveInit`, `Drop`, `SetDrop`, `Destroy`, `MemCpy`,
`MemSet`, `FunctionAddr`, `GlobalAddr`, `Init`, `CallIndirect`, `Br`, `CondBr`,
`Unreachable`, `SizeOf`, `AlignOf`, `OffsetOf`, `DynamicCall`, `Async*`,
`InlineAsm`. (`Gep`, `Select`, `Switch`, `Throw` are declared but **not produced
by the lowerer today** — see §5.4.)

---

## 4. Target architecture

```
                      ┌───────────────────────────────┐
   AST (resolved,     │  MIRLowerer  (AST → MIR)      │
   type-checked) ────►│  — the ONE lowering —          │
                      └───────────────┬───────────────┘
                                      │  MIRModule
                    ┌─────────────────┼───────────────────┐
                    ▼                 ▼                   ▼
             MIREmitter(C)      CodegenMir(LLVM)    MIRInterpreter
                                                          │
                                             (run: --arg-interpret,
                                              then: comptime values)
```

The engine executes a single, fully-populated `MIRModule`. It is single-threaded
(no locks anywhere at run time) and reads only MIR tables.

### 4.1 Execution model

MIR is already an explicit, C-like memory IR (places, loads/stores, byte copies,
sret), so the engine is a *faithful executor of the C semantics the emitter
prints*:

- **Values** are 64-bit cells (`uint64_t bits`). Scalars are stored in the low
  bits; pointers are addresses. **Aggregate values are addresses** into frame
  memory (a value of aggregate type is a pointer to bytes).
- **Places** are storage of `MIRTypeRecord.size` bytes, allocated from a per-call
  bump arena. A place is addressable at a stable address for its lifetime.
- **Load/Store** read/write the bytes; `Load` of an aggregate **copies** into
  fresh frame storage (matching `struct T v = p;`), `AddressOf` yields the place
  address.
- **Calls** follow `MIRABI`: aggregate return via sret (the caller's result place
  address is arg 0), aggregate args as pointers. `Param` instructions consume the
  incoming argument cells in order.
- **Globals** own persistent storage; initialized from `MIRConstExpr` at load
  time (§5.2).
- **Externs/host functions** (libc and remaining intrinsics) are resolved once to
  native function pointers (§5.3).

### 4.2 Cross-module resolution

Each `implement_module` normally owns its own type/symbol tables. For
interpretation the whole job shares **one** `MIRModule` (and one arena) so ids are
consistent across modules; anything not deduplicated by id (symbols/globals,
which the lowerer may intern more than once) is resolved **by mangled name**.
This mirrors what the linker does for C/LLVM and is the simplest correct model.

---

## 5. Performance architecture (this is the core of the plan)

The interpreter is a hot loop. Every rule below exists to keep the steady-state
cost of one MIR instruction close to one C statement.

### 5.1 Resolve everything at load time, never in the loop

Build **flat, dense side tables** once, when the program is loaded, and index
them by the same dense ids MIR already uses. The exec loop must never hash a
string, take a lock, or consult `std::shared_mutex`.

| Fact | Side table | Built from |
|---|---|---|
| type size, kind, signedness, `is_float`, `is_pointer`, `is_aggregate` | `std::vector<TypeInfo>` indexed by `TypeId` | `MIRTypeTable` |
| field name → `(offset, TypeId)` per aggregate | open-addressed map per `TypeId`, or a precomputed map for the few aggregates that use names | `MIRFieldInfo` |
| symbol name → `MIRFunction*` or host fn ptr | `std::vector<Callee>` indexed by `SymbolId` | `MIRSymbolTable` + `functions` |
| global symbol → storage pointer | `std::vector<uint8_t*>` indexed by a dense global index | `MIRGlobal` |
| `Constant` (int/float/bool/null) → bits | `std::vector<uint64_t>` indexed by `ConstantId` | `MIRConstantPool` |
| `ConstString` → NUL-terminated `char*` | `std::vector<const char*>` indexed by `ConstantId` | `MIRConstantPool` |
| `FieldAddr`/`Load`-field/`Store`-field with a **Constant** field operand → byte offset | patched into the instruction's operand (or a parallel `std::vector<uint32_t>` keyed by instruction index) at load time | `MIRFieldInfo` |

> Do this in one "prepare" pass over `module.functions` before execution. It is
> O(program size), single-threaded, and removes all name/dictionary work from the
> hot path.

### 5.2 Constants and globals

- Pre-materialize every `MIRConstantKind::String` into a NUL-terminated buffer
  once (C string literals are NUL-terminated; the MIR pool is not). Cache by
  `ConstantId`.
- Evaluate each `MIRGlobal` initializer tree **once** into its storage. Strings
  store the pointer; aggregates/arrays evaluate recursively by field offset /
  element stride. No const-expr evaluation at run time.

### 5.3 Calls

- Resolve `Call`'s `Symbol` to a `Callee` record at load time:
  `{ enum { Mir, Host }, const MIRFunction* fn, HostFn host }`.
- `Mir` calls into the engine; `Host` is a native C function pointer. No symbol
  string is ever touched at call time.
- Host bindings needed by the interpret suite (all trivially wrappable):
  `printf`, `malloc`, `free`, `realloc`, `memcpy`, `memset`, `strlen`, `strcmp`,
  `memmove`, `abort`, `exit`, and (if still emitted) the intrinsics that survive
  lowering.
- **`printf`/varargs**: the engine reads the format string and the typed operands
  from the call's operand list, because MIR operand types are known. Implement a
  small formatter over `%d/%i/%u/%x/%s/%c/%p/%%` rather than forwarding to libc
  `va_list` (we cannot reconstruct a `va_list` portably). This is also what lets
  the interpret suite print identically to compiled mode.

### 5.4 Dispatch loop — the single hottest thing

- Use **GCC/Clang computed goto** (`&&label` + `goto *dispatch[opcode]`) when the
  compiler supports it, and a plain `switch` otherwise. Computed goto typically
  removes a large fraction of the branch-mispredict cost of a `switch` on a
  random opcode stream; the source stays a macro so both forms exist.
- Precompute a **per-instruction descriptor array** at load time if profiling
  shows operand decoding is hot: a compact `struct { uint8_t op; uint8_t nops;
  }` avoids re-decoding the instruction header each visit.
- Cache `const MIRInstruction*` and the function's `values`/`places` base
  pointers in locals at block entry so operand access is a couple of indexed
  loads.
- Keep the current block's `inst_end` in a local; branch by writing the block id
  and `continue`-ing the outer loop, not by recursive calls.

### 5.5 Memory and frames

- **One bump arena per call frame**, reset when the call returns (the C stack
  analogue). No `malloc`/`free` per place. Deep recursion gets a frame stack;
  keep frames in a `std::vector<Frame>` reused across calls (do not reallocate
  the value/place vectors when the capacity is enough — `clear()` and reuse).
- **Value slots** are one `std::vector<uint64_t>` per frame sized to
  `fn.values.size()`. **Place storage** is arena offsets, or a
  `std::vector<uint8_t*>` sized to `fn.places.size()` for simplicity first,
  densified only if it shows up.
- Aggregates are byte blobs in the arena; copies use `memcpy` with the known
  `TypeInfo.size`.
- Do **not** zero an aggregate `Alloca` implicitly. The lowerer emits `MemSet`
  exactly where C semantics require defined bytes (see `MIREmitter` `Alloca`
  comment). Mirroring this keeps the copies and zeroing identical to C.

### 5.6 Arithmetic, compared to C

- Compute in 64-bit then truncate to the result type's width and sign
  (`int8_t r = (int8_t)(a + b)`). This is what the emitter prints and what the
  AST interpreter's `coerce_to_type` emulates; doing it natively is exact and
  fast.
- Branch on `is_float`/`is_double` once per arithmetic op; avoid `switch` inside.
- `Cast` is a table of small inline functions keyed by `(src_kind, dst_kind)`.
- Keep the interpreter free of `std::function`, `std::string`, and virtual
  dispatch in the loop.

### 5.7 Explicitly forbidden in the hot path

- `std::shared_mutex` / any lock (the interpreter is single-threaded).
- `std::unordered_map` lookups, string compares, `std::string` construction.
- Re-deriving layout, ABI or field offsets from names.
- Touching the AST.
- Per-instruction heap allocation.

---

## 6. Acceptance criteria (gates)

Run after **every** phase unless marked otherwise.

1. **Compiled-C invariance.** Emit main-suite C and compare against the frozen
   serial baseline (`lang/tests/build/chemical-tests.dir/Translated.c`), same
   normalizer used for the MIR perf work. Byte-identical modulo the known
   `__cgs__N`/`__cfg__N`/`__chemda_*`/`__chx_*N` normalizations.
2. **Suites green.** `MIRTests`; `./scripts/test.sh --tcc`; `--llvm`;
   `--tcc --interpret` (once the job is ported); then `--libs`, `--async`.
3. **Interpret correctness.** `--tcc --interpret` passes the same set of tests
   the AST interpreter passed (or the tests whose branches were pinned to the
   AST interpreter are updated to the unified compiled semantics, §7.0).
4. **Interpret performance.** Median wall-clock of `./scripts/test.sh --tcc
   --interpret --no-build` is within ~10 % of the AST interpreter baseline
   recorded at Phase 0. Record the number in `compiler/mir/README.md`.
5. **No AST in the engine.** `grep -R "ast/" compiler/mir/MIRInterpreter*` is
   empty; the engine compiles in the `MIRTests` core target.

---

## 7. Phases

Each phase is a standalone, reviewable commit series. Land in order.

### 7.0 — The one semantic decision (made in Phase 0, applied from Phase 4)

The interpretation job currently lowers with `interpretation_mode = true`, which
makes `intrinsics::is_comptime()` and `intrinsics::is_interpretation()` true and
selects the `expr_println` branch in `lang/tests/common/src/test.ch`.

For the MIR job, **lower with `interpretation_mode = false`.** Interpretation
then takes the exact branch compiled mode takes (`printf`), `expr_println` never
enters MIR, and the engine only needs a `printf` host binding. This is the whole
point of the migration: interpretation becomes the *same semantics* as compiled
mode, executed by a different engine. The handful of tests that pin
interpretation-only behaviour (`comptime/intrinsics.ch` `is_interpretation` /
`is_runtime` branches, `destructors/capturing_call.ch`) are updated in the same
commit to the unified expectation, because after this change there is exactly one
semantic answer.

### Phase 0 — Baseline + harness

- Record baselines: `--tcc --interpret` wall time and pass count; main-suite
  emitted C; `MIRTests`.
- Add a **MIR round-trip smoke test** to `MIRTests`: lower a tiny hand-built
  `MIRFunction` (or a `lang/compiled/` fixture) and run it in the engine.
- Decide and document the one semantic change: for the interpretation *job*,
  lowering runs with `interpretation_mode = false` (§7.0).

**DoD:** baselines committed to `compiler/mir/README.md`; smoke test runs.

### Phase 1 — MIR collect mode for the job

Goal: the interpretation job ends with a **fully-populated shared `MIRModule`**.

- Add `mir::MIRInterpProgram` (`compiler/mir/MIRInterpProgram.h`):
  `MIRModule module; unique_ptr<MIRTypeBuilder> types; unique_ptr<MIRTaskContext>
  task; recursive_mutex visitor_mutex;` — the persistent tables + arena for the
  whole job.
- Refactor `ASTProcessor::implement_module` to use caller-provided
  module/types/task/mutex when an interpreter context is set (locals otherwise).
  Minimal, mechanical change; the C/LLVM paths keep the locals.
- In `do_interpretation_job`, after symres/typecheck, run
  `declare_module` + `implement_module` per module with the collect flag and the
  shared context (mirroring `ASTProcessor::mir_compile_module`), then proceed.
- Globals: `process_nodes` already calls `lowerer.build_global_initializer` for
  top-level `VarInitStmt`; verify every global the job needs lands in
  `MIRModule::globals` (including `const ANSI_COLOR_*` strings and the
  `total_tests` counters).

**DoD:** after the job's module loop, `program.module.functions` contains a
`MIRFunction` for `main`, `test`, `print_test_stats`, …; `globals` contains the
counters and color constants. No behaviour change yet (job still AST-interprets).

### Phase 2 — The execution engine, scalar/control core

`compiler/mir/MIRInterpreter.{h,cpp}` (AST-free, `MIRTests`-linked).

- Prepare pass: build all §5.1 side tables.
- Frame: values + place storage + bump arena + block dispatch.
- Opcodes (scalar/control first): `Nop`, `Const*`, `Alloca`, `Param`, `Load`
  (all forms), `Store` (all forms), `AddressOf`, `FieldAddr`, `IndexAddr`,
  `Unary`, `Binary`, `Compare`, `Cast`, `SizeOf`, `AlignOf`, `OffsetOf`,
  `Br`, `CondBr`, `Return`, `Unreachable`.
- Call/return for **scalar** functions; `Param` binding; sret plumbing
  (allocate + pass the result address).
- Const/gloal materialization (§5.2).
- Computed-goto dispatch behind a macro.

**DoD:** a hand-written module (`lang/compiled/mir_interp_smoke/`) that does
integer/float arithmetic, branches, and a scalar function call runs and prints
the same results as `--tcc`.

### Phase 3 — Aggregates, aggregates-by-value, lifetime, patterns

- Aggregate value semantics: aggregate value = address; `Load` copies; `Store`
  copies; `CopyInit`/`MoveInit`/`Assign`; `MemCpy`/`MemSet`.
- `Init` (constructor calls), `Drop`/`Destroy`/`SetDrop` (run the destructor
  function through the engine), `FieldAddr`/`IndexAddr` with real offsets, array
  decay (`AddressOf` of an array).
- `GlobalAddr`, `FunctionAddr`, `CallIndirect`.
- Struct defaults, variant construction/switch (name-path field offsets,
  `Case.param` dotted names).
- Host bindings: `printf` (+ formatter), `malloc`/`free`/`memcpy`/`memset`/`strlen`.

**DoD:** the `common_tests` bodies run under the engine for a first
representative slice ("arithmetic / loops / structs / variants / destructors"),
printing via `printf`.

### Phase 4 — Wire `do_interpretation_job` to the engine

- Replace the AST `VarInitStmt` walk + `main_fn->call(...)` with:
  1. prepare the engine over the shared module,
  2. evaluate `MIRGlobal`s,
  3. call the `MIRFunction` for `main`.
- `is_interpretation()`/`is_comptime()` must be resolved to the **compiled**
  answers during the job's lowering so `test.ch` selects `printf` (§7.0). The
  engine then implements `printf`.
- Output must match the AST interpreter byte-for-byte (ANSI codes included) or
  the test expectations are updated in the same commit.

**DoD:** `./scripts/test.sh --tcc --interpret` runs through the engine and is
green; performance gate §6.4 met.

### Phase 5 — Comptime on MIR, then delete the AST interpreter

- Hook `MIRLowerer`'s comptime callbacks (`set_comptime_eval`,
  `set_comptime_value_eval`, `set_comptime_ctor_eval`, `set_comptime_block_eval`,
  `set_comptime_if_resolver`) so `comptime` function calls and `%runtime_value`
  are evaluated by **lowering the comptime callee to MIR and running it in the
  engine**, rather than by the AST interpreter.
- Keep a tiny AST shim only if a genuinely pre-MIR need remains
  (`comptime if` branch selection during declaration, array-length constants —
  `IfStatement::get_condition_const`, `SymResLinkBody.cpp:2562`,
  `LinkSignature.cpp:644`). If the condition is lowered to MIR and run, even
  that shim can go; evaluate case-by-case and document what remains and why.
- Delete `compiler/Interpreter/Core.cpp`, the `evaluated_value` chain, and the
  interpreter-friendly std types — after confirming no caller remains
  (`grep` for `evaluated_value`, `InterpretScope`, `FunctionDeclaration::call`).

**DoD:** no AST-interpretation execution path remains; all suites green;
`--interpret` performance gate still met.

---

## 8. Verification matrix

| Phase | `MIRTests` | `--tcc` | `--llvm` | `--interpret` | C invariance | perf gate |
|---|---|---|---|---|---|---|
| 0 | ✔ | ✔ | ✔ | AST (baseline) | ✔ | baseline recorded |
| 1 | ✔ (extends) | ✔ | ✔ | AST (unchanged) | ✔ | — |
| 2 | ✔ + smoke | ✔ | ✔ | AST | ✔ | — |
| 3 | ✔ + smoke | ✔ | ✔ | AST | ✔ | — |
| 4 | ✔ | ✔ | ✔ | **engine** | ✔ | ✔ |
| 5 | ✔ | ✔ | ✔ | engine (no AST) | ✔ | ✔ |

---

## 9. Risks & mitigations

1. **The `comptime if` / `expr_println` trap.** `intrinsics::expr_println` is a
   *comptime* `FunctionDeclaration`; if it survives into MIR the lowerer
   evaluates it at **lowering time**, printing once regardless of the runtime
   branch. **Mitigation:** §7.0 — the interpretation job lowers with
   `interpretation_mode = false`, so `test.ch` takes the `printf` branch and
   `expr_println` never enters MIR. This is also the semantically *correct*
   target (interpretation == compiled).
2. **Cross-module ids.** Symbol/global interning can produce duplicate ids for
   one name. **Mitigation:** resolve functions and globals by mangled name in the
   prepare pass; the engine never assumes ids are globally unique.
3. **Aggregate value semantics drift.** `Load`-copies vs aliasing, empty
   aggregates (size 0), array decay. **Mitigation:** the engine mirrors the
   emitter's decisions one-for-one; the `MIRTests` smoke tests compare against
   the compiled executable.
4. **Performance regressing silently.** **Mitigation:** the perf gate in §6.4 is
   measured every phase and recorded; the forbidden list in §5.7 is a review
   checklist.
5. **Scope creep into async.** The engine must handle `Async*` opcodes only as
   "unsupported during interpretation" (they are not produced for the interpret
   suite); do not build a coroutine executor.
6. **Deleting the AST interpreter too early.** Phase 5 is last, and only after
   the engine covers the suite; the AST interpreter is the reference oracle for
   Phase 3–4 output comparison.

---

## 10. Open questions (resolve during Phase 1/2)

- Single shared `MIRModule` vs. per-module modules + name remap. The plan picks
  the shared module for simplicity; confirm `file_level_reset()` /
  `mir_types.clear_memo()` per file are safe with a shared, never-reset module
  and arena (they are expected to be, since MIR stores no `BaseType*`).
- Whether `Gep`/`Select`/`Switch`/`Throw` ever appear under different options or
  future lowering; today they do not. Implement them defensively or fail loudly.
- `DynamicCall`/vtables: the interpret suite does not use `dyn` today. If it
  gains a user, route `DynamicCall` through the same vtable records the LLVM
  backend consumes (`MIRVtableTable`), not through the AST.
- Whether to densify place/global storage further (offsets instead of pointers)
  after a first profiling pass.
