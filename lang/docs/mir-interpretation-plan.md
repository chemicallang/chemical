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

**Status: not started. This is the remaining work and it is a migration, not a
patch.** The interpretation job's *runtime* is on MIR (§ Phase 4), but the
compiler still uses the AST interpreter for **comptime evaluation during
lowering and symres**, which is on every compile path (C, LLVM, interpret, and
CBI plugins). Deleting the AST interpreter requires moving all of that first.

#### 5.1 The exact surface (inventory, verified in the tree)

The only callers of the AST interpreter outside `ast/values/*` and
`ast/structures/FunctionDeclaration.cpp` (the interpreter's own implementation)
are:

| Caller | What it evaluates |
|---|---|
| `MIRLowerer::set_comptime_eval` → `CTranslationContext::eval_comptime_call` | a `comptime` callee (`lower_call`/`lower_method_call`/ctor paths) |
| `MIRLowerer::set_comptime_value_eval` | a `Value*` (`v->evaluated_value`) — used for `%runtime_value`, index/field comptime reads |
| `MIRLowerer::set_comptime_ctor_eval` → `eval_comptime_ctor` | a comptime constructor |
| `MIRLowerer::set_comptime_block_eval` | a `comptime { … }` block (`comptime_scope.interpret`) |
| `MIRLowerer::set_comptime_if_resolver` → `IfStatement::get_or_resolve_scope` | a `comptime if` condition |
| `MIRDeclEmitter::set_value_eval` (`ASTProcessor.cpp`) | the front-end declaration pass's `Value*` evaluation |
| `SymResLinkBody.cpp:2562`, `LinkSignature.cpp:644` | `array_size_value->evaluated_value` |
| `compiler/Interpreter/Core.cpp` | the interpreter itself (statements, loops, scopes) |

`CTranslationContext::eval_comptime_call` funnels to
`evaluate_comptime_func` → `FunctionDeclaration::call(&comptime_scope, …)`, and
then `result->evaluated_value(comptime_scope)` for a `RuntimeValue`.

CBI macro plugins do **not** embed the interpreter (they are TCC-compiled from
`lang/libs/compiler` and call compiler callbacks); the interpreter runs inside
the compiler only. So plugins are not a separate blocker — but every callback
above runs *during lowering*, which is the same lowering plugins trigger.

#### 5.2 Why this is hard (the real design problem)

A MIR interpretation of a comptime function returns a **raw** value, but every
call site needs an AST `Value*` that lowering then folds. Concretely:

1. **Result bridge.** Scalar results (`int/u*/f32/f64/bool/pointer`) map to
   `IntNumValue`/`FloatValue`/`DoubleValue`/`BoolValue`/`NullValue` directly.
   Aggregate results (structs, variants, arrays) must be rebuilt as
   `StructValue`/`ArrayValue` trees — which is most of what the interpreter's
   `StructValue::initialized_value` does today. That bridge is the bulk of the
   work and must be exact (field paths, variant tags, nested destructible
   fields).
2. **Argument lowering.** A comptime call's arguments are AST `Value*`s that
   may reference the *enclosing* function's locals. Lowering them to raw MIR
   argument values needs an execution context that can see the enclosing
   frame, i.e. the comptime evaluation is not an isolated function call — it
   is a nested execution of the same program. (Comptime args are usually
   literals/consts, but not always.)
3. **Re-entrancy.** The comptime callbacks fire *while* `MIRLowerer` is mid
   function. Lowering the comptime callee needs a separate lowerer/arena, but
   `MIRLowerer` is stateful (bindings, current function, `builder_`) and the
   callbacks capture the shared `CTranslationContext`. `copy_callbacks_from`
   gives a nested lowerer the callbacks, but not the enclosing frame.
4. **Memory shape.** `MIRInterpreter` today runs a whole `MIRModule` from
   `main`. Comptime needs "run one function with these argument bits and give
   me the result bits", plus a way to hand back an aggregate. (The engine's
   internals already support this; it needs a public entry point and a
   result-materialisation contract.)

Because of 1–3, the honest approach is **not** "call `MIRInterpreter` from
`eval_comptime_call`". It is one of:

- **(A) Nested execution with a shared frame.** Extend the engine to run a
   comptime `MIRFunction` against the current execution frame (so argument
   values are read from the live frame), and add the `Value*` result bridge.
   Biggest change, but preserves the semantics exactly.
- **(B) Two-phase comptime.** Lower comptime-annotated functions eagerly to MIR
   before the normal pass, run them with a dedicated context, and cache results;
   then the normal lowering reads cached constants instead of calling the AST
   interpreter. Cleaner separation; needs a new pass and a comptime dependency
   order.
- **(C) Keep a minimal AST evaluator.** Retain only `Value::evaluated_value`
   for `Value*` shaping (the result bridge) and delete the statement/scope
   interpreter (`compiler/Interpreter/Core.cpp`), the `FunctionDeclaration::call`
   body, and the interpreter-friendly std types. This is the smallest safe
   reduction and is a legitimate intermediate state, but it does **not** remove
   `evaluated_value`.

#### 5.3 Worklist (incremental, each step keeps every suite green)

1. **Engine entry point.** Add `MIRInterpreter::call_function(name, args,
   nargs, out_result_bits, out_aggregate_bytes)` so a comptime driver can run
   one function in isolation. Pure addition; no behaviour change.
2. **Result bridge, scalars first.** A `Value*` builder for scalar MIR results
   (`IntNumValue`/`FloatValue`/`DoubleValue`/`BoolValue`/`NullValue`).
3. **Route `set_comptime_value_eval` for scalar-only `Value*`s** through the
   engine, with the AST path still used for aggregates; measure how many
   callbacks that covers (instrument, don't guess).
4. **Aggregate bridge.** Rebuild `StructValue`/`ArrayValue`/variant values from
   MIR bytes using `MIRTypeTable` layout (field paths already resolve in the
   engine).
5. **Comptime driver (approach A or B).** Resolve the re-entrancy/argument
   problem above; switch `set_comptime_eval`/`_ctor_eval`/`_block_eval` and the
   two symres array-size sites to it. `set_comptime_if_resolver` last.
6. **Delete.** Remove `compiler/Interpreter/Core.cpp`, the `evaluated_value`
   overrides, `InterpretScope`/`GlobalInterpretScope`, `FunctionDeclaration::call`
   (keep only the declaration), and the interpreter-friendly std types. Confirm
   with `grep -R "evaluated_value\|InterpretScope\|->call(&" compiler/ ast/`.

**DoD:** no AST-interpretation execution path remains; all suites green;
`--interpret` performance gate still met.

> Do **not** attempt steps 4–6 before steps 1–3 are green. The comptime path is
> on every build; a partial migration that changes comptime results silently
> changes every backend's output.

##### 5.3.0 Measured surface (2026-10, `CT_STAT=1` on the test-suite compile)

`set_comptime_eval`/`_value_eval`/`_ctor_eval`/`_block_eval` are env-gated
counters (`ct_stat*` in `compiler/ASTProcessor.cpp`). On `lang/tests/build.lab`:

| callback | calls | what it is |
|---|---:|---|
| `set_comptime_eval` (CE) | 3517 | 108 distinct callees |
| `set_comptime_value_eval` (CV) | 3195 | `Value*` expression folds |
| `set_comptime_ctor_eval` (CC) | 0 | all ctors are templates now |
| `set_comptime_block_eval` / `_if_resolver` | 0 | not hit by this compile |

**The key finding:** almost all CE calls are **compiler intrinsics** — C++
`FunctionDeclaration::call` overrides that execute no statements and therefore
*already* survive as the §5.4 residual:

| callee | calls |
|---|---:|
| `intrinsics::size` | 1385 |
| `satisfies` | 318 |
| `get_loc_file_path` / `get_call_loc` | 237 / 237 |
| `get_lambda_fn_ptr` / `_cap_ptr` / `sizeof_lambda_captured` / `_cap_destructor` / `alignof_lambda_captured` | 149 / 149 / 145 / 145 / 145 |
| `decode_location` | 134 |
| `expr_str_block_value` | 50 |
| `is_same_type` / `value_satisfies` | 48 / 26 |
| `comptime_size_of` / `comptime_align_of` / `defined` / `atomic_fence` / `get_single_marked_decl_ptr` | 20/18/4/6/4 |

Only **~250** CE calls reach the **statement** interpreter: `comptime_fnv1_hash`
(57), `Layout::of` (16), `comptime_func_call_gen1` (16), `compare_enum_result7`
(12), `comptime_give_char_by_offset(s)` (12), the `comptime/basic.ch` fixtures
(`comptime_primitive`, `comptime_logical_*`, `pair_66`, `comptime_sum`, …), and
`Pair66`/`comptime_func_container` methods. CV is similarly expression folding
(`AccessChain`, `String`, `LambdaFunc`, `IntN`) — §5.4 says it survives.

So "delete `compiler/Interpreter/Core.cpp`" is bounded by **~50 distinct pure
comptime functions + the aggregate result bridge**, not by the whole language.

##### 5.3.1 Driver status (2026-10)

`compiler/mir/MIRComptime.{h,cpp}` implements approach B for the lowering-time
comptime call. It lowers the callee **and every function it reaches,
transitively**, into one shared MIR module, prepares `MIRInterpreter` once per
growth, and calls the callee by mangled name; the raw 64-bit result is bridged
back to an AST value (`IntNumValue`/`BoolValue`/`NullValue`; aggregates pending).

It **declines** anything whose interpreter semantics the engine does not
reproduce, so those keep the AST path and every suite stays green. On the
test-suite compile it handles **441** comptime calls (~98% of the real ones) —
`comptime_fnv1_hash`, `decode_location`, `comptime_size_of`, `Layout::of`, the
`comptime/basic.ch` fixtures, `Pair66` constructors, …

Covered since the first cut:
1. **Generics.** Unique mangled names inside the driver's module — generic
   instantiations share the host's mangled name, which the module-local
   `fn_by_name` lookup collided on.
2. **Flat-struct results** (`MIRInterpreter::call_sret` + `MIRTypeTable` layout
   read back into a `StructValue`); a self-referencing struct type
   canonicalizes to a `LinkedType`, so the return check resolves the linked node.
3. **No-body callees** (intrinsics the lowerer folds, externs the engine binds as
   hosts) — declined only when they *diverge* (`is_comptime`/`is_runtime`,
   call-site/location introspection, `intrinsics::vector`).
4. **Expressions** — declined only for string `+` (the interpreter's
   concatenation); pointer arithmetic/comparison and string-literal equality
   (identical contents are interned) match the engine.
5. **Comptime-mode intrinsics** — `is_comptime`/`is_runtime`/`is_interpretation`
   fold in the lowerer from the function being lowered (interpretation now has
   the same semantics as compiled mode, so `is_interpretation` is false).

Green: `--tcc` 2307, `--interpret` 1896, `--llvm` 2305.

Remaining to retire `compiler/Interpreter/Core.cpp` (measured residual, **10
calls**):
1. **Array/slice results** (2: `get_tests`) — the `ArrayValue` bridge.
2. **Call-site location** (2: `give_caller_line_no`) — the engine needs the
   enclosing call's site.
3. **Method receivers that are runtime values** (2:
   `comptime_func_container::give_arg`) — the receiver is a runtime operand, so
   it cannot be materialized at comptime; the engine would need a runtime
   receiver, or the caller must pass it.
4. **`intrinsics::vector`** (4: `compiler_vector_sum*`) and **string `+`** (2:
   `check_addition_strings`/`comptime_add_strings`) — **interpreter-only
   language features** exercised by `comptime/vector.ch` and
   `comptime_add_strings`. Deleting the interpreter requires a decision: port
   them to the engine, or migrate/remove those tests (string `+` is
   pointer-arithmetic on the compiled backends today).
6. **Delete** — `compiler/Interpreter/Core.cpp`, `InterpretScope`/
   `GlobalInterpretScope`, `FunctionDeclaration::call`'s body execution, and the
   interpreter-friendly std types; `Value::evaluated_value` survives as the §5.4
   symres-time expression folder.

#### 5.4 The unavoidable residual (why "delete the AST interpreter" has a floor)

`symres` resolves a top-level `comptime if` **before MIR exists** —
`IfStatement::get_condition_const` calls `condition->evaluated_value(scope)`,
and the condition is almost always a call to a **compiler intrinsic** whose body
is a C++ `FunctionDeclaration::call` override (`intrinsics::is_comptime`,
`is_runtime`, `is_interpretation`, `is_target`, `is_clang`, `satisfies`,
`get_child_fn`, `get_lambda_fn_ptr`, `get_raw_location`, …). There is no MIR at
that point and there never can be: the intrinsic's result decides which branch
even exists, which decides what to lower. The same is true of the two
array-length sites (`SymResLinkBody`, `LinkSignature`).

So Phase 5's realistic end state is **not** "zero AST evaluation". It is:

- **Lowering-time comptime** (functions, blocks, `%runtime_value`, ctors) → MIR
  engine (steps 1–5).
- **A residual, symres-time evaluator** for `comptime if` conditions, array
  lengths, and the compiler intrinsics → kept, but shrunk to
  `Value::evaluated_value` plus the intrinsic `call` overrides. The statement
  interpreter (`compiler/Interpreter/Core.cpp`), `InterpretScope`,
  `FunctionDeclaration::call`'s body execution, and the interpreter-friendly std
  types can then go — because nothing left *runs statements*; only *expressions*
  are folded.

The revised DoD: `compiler/Interpreter/Core.cpp` and the statement/scope
interpreter are gone; `Value::evaluated_value` survives only for
symres-time constant folding and intrinsic evaluation; all suites green. Truly
deleting `evaluated_value` would require re-resolving `comptime if`/array
lengths *after* a first lowering pass — a larger compiler restructuring, out of
scope for this plan.

**Recommendation:** approach **(B) two-phase comptime** is the least risky path
to the revised DoD: it isolates comptime to its own pass (no re-entrancy into a
live lowering frame), gives the `Value*` bridge a single place to live, and lets
each backend read folded constants instead of calling into the interpreter.



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
