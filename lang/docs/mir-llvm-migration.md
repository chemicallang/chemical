# MIR → LLVM Migration — Handoff, Plan & Contract

> **Status: PLANNED (not started).** This document is the contract for the work.
> Read it in full before changing anything.
>
> Living/related docs:
> - `compiler/mir/README.md` — MIR status, gotchas, repro commands (the living doc).
> - `lang/docs/mir-design.md` §10 (LLVM Backend), §7 (Cleanup/Destructors), §3 (types).
> - `lang/docs/mir-implementation-plan.md` §0 (MIR-first mandate) and **Stage 8**.
> - `.agents/skills/mir/SKILL.md` — quick orientation for agents.
>
> Baseline commit at the time of writing: **`8001ecde3`** on branch `mir`.

---

## 0. How to resume in a new session (copy/paste prompt)

Paste this to a fresh AI session (fill in the `TASK` section):

```text
You are continuing the MIR → LLVM migration on branch `mir` (repo:
Chemical/chemical). Before doing anything, read these in full:
  1. lang/docs/mir-llvm-migration.md        (this file — the contract)
  2. compiler/mir/README.md                  (MIR status + gotchas)
  3. lang/docs/mir-design.md §7, §10, §3     (semantics + LLVM backend)
  4. lang/docs/mir-implementation-plan.md §0 + Stage 8 (MIR-first mandate)
  5. .agents/skills/mir/SKILL.md

Baseline: commit 8001ecde3. MIR currently emits C only; the LLVM backend still
lowers directly from the AST and must be deleted and re-implemented as MIR → LLVM.

Binding rules (from §0 of the plan, restated here):
  - No feature flags, no `--use-mir`, no capability analysis, no legacy fallback.
  - Delete the AST-based LLVM lowering; do not keep it behind a flag.
  - Debug information (DWARF) must be preserved through MIR for LLVM emission
    when the user opts in (see §5.1). Never drop source locations.
  - Generated C and LLVM must stay functionally equivalent to the current MIR/C
    behavior and to the old LLVM behavior.
  - MIRTests must stay green; the TCC suites must stay green.
  - If you find a MIR/common bug that affects a core language feature (it would
    affect the C backend too), FIX it AND add a regression test in
    lang/tests/common/src (see §8).

TASK: <describe the single next checkpoint, e.g. "Stage 8.2: lower aggregate
layout into MIRTypeRecord and add MIR→LLVM type lowering for scalars/pointers">.

Before finishing, run the verification commands in §9 relevant to your change
and report the exact pass/fail numbers. If the LLVM suite cannot run, say why.
```

**Rules for the human continuing:** give the AI **one checkpoint at a time**
(§7). Ask it to end every turn with (a) what it changed, (b) the exact commands
it ran and their numbers, (c) the next checkpoint. If it stalls or produces a
huge diff, point it back at this document and at `compiler/mir/README.md`.

---

## 1. Mission

Make **MIR the single lowering for every backend**. Concretely:

1. **Delete the AST-based LLVM lowering.** Remove the per-AST-node LLVM codegen
   (`llvm_value` / `llvm_pointer` / `llvm_type` / `code_gen*` under
   `#ifdef COMPILER_BUILD`), the `Codegen` AST driver, `backend/LLVM.cpp`,
   `backend/LLVMCoroutine.cpp`, and the AST-shaped parts of `DebugInfoBuilder`.
2. **Emit LLVM IR from MIR.** Add a `MIRLLVMEmitter` that consumes `MIRFunction`
   / `MIRModule` and produces LLVM IR, equivalent to today's LLVM output.
3. **Keep debug info.** When the user opts in (`--mode debug_complete` / `-g`),
   the emitted LLVM IR must carry DWARF debug info, now sourced from MIR
   metadata (§5.1). This is a hard requirement, not optional.

This is the already-planned **Stage 8** of `mir-implementation-plan.md`. The
MIR-first mandate (§0) already says "always lowers … to MIR and always emits C
(and later LLVM) from MIR". This document is the execution contract for the
"later LLVM" part.

---

## 2. Current state / baseline

### 2.0 Progress log (live)

- **Foundation started (uncommitted).** `compiler/mir/MIRTypeTable.h` /
  `MIRTypeBuilder.{h,cpp}` now fill ordered runtime **aggregate field tables**
  (structs, unions, variants with tag + per-case payload structs, enums) and a
  new `compiler/CodegenMir.cpp` (COMPILER_BUILD-only) implements the first
  `MIR → LLVM` lowering: `mir_type` (all MIR type kinds), `mir_function`
  (declare), `mir_declare_globals`, and `mir_emit_function` for the core opcodes
  (const, alloca, load/store, address-of, field/index address, unary/binary/
  compare, cast, select, sizeof/alignof/offsetof, call/call-indirect,
  init/copy-init/move-init/assign, drop/destroy/set-drop, memcpy/memset,
  br/cond-br/return/unreachable).
- **Driver wired (for the LLVM target):** `ASTProcessor::mir_compile_module`
  (in `compiler/ASTCompiler.cpp`) builds the MIR module from the resolved AST
  (types, symbols, top-level + container methods) and emits LLVM from MIR.
  `compile_module` calls it; the AST lowering below is retained only as a
  fallback until every construct is covered.
- **Verified working end-to-end** on no-import probe modules (compile with
  `--assertions`, run): scalar functions + calls, `for`/`while` control flow,
  struct construction + field access, arrays + array parameters (decay),
  pointers + pointer arithmetic, struct pointers, methods, `@delete`
  destructors, enums + `switch`, variants + pattern matching, and globals.
  Example output: `alive 7 / dtor 7 / done`, `g=5 c=200 300 / m=42 n=-1`.
- **Known gaps found while probing:** the variant LLVM layout currently
  concatenates case payloads instead of a union (functionally correct for the
  self-consistent emitter, but larger than the C layout); interfaces /
  `DynamicCall` (needs an LLVM vtable type + global declaration pass that MIR
  does not yet carry), capturing functions / lambdas, CBI embedded nodes, inline
  asm, async, and debug info are not lowered yet.
- **Also fixed this session:** global *initializers* are now applied from AST
  literals (`getNamedGlobal`, not `getGlobalVariable` — the latter skips internal
  globals); bool→int casts zero-extend; unsigned `div`/`rem`/`shr`/comparisons
  use the unsigned LLVM opcodes; the `__chemical_fat_pointer__` type now has its
  `first`/`second` pointer fields.
- **Tree health:** the `Compiler` target builds; the TCC main suite is
  unaffected (all changed code is COMPILER_BUILD-only) and was green
  (2292/2292) after the MIR layout change.
- **Not done yet:** interfaces / `DynamicCall` / vtables, capturing functions /
  lambdas, CBI `EmbeddedNode`/`EmbeddedValue`, inline asm, async coroutines,
  debug info, and deleting the AST LLVM lowering (172 files / ~10k lines).
- **Atomics lowered to first-class MIR ops (this session).** `intrinsics::llvm::atomic_*`
  used to be *interpreted* during MIR construction; that path needs a live LLVM
  `IRBuilder` and a real `llvm::Value` per runtime pointer, so it dereferenced a
  null `llvm::Value` and SIGSEGV'd inside `LLVMBackendContext::atomic_*`
  (`std_stdstring_viewconstructor`-style verifier errors followed). Atomics are
  now MIR:
  - New opcodes `AtomicLoad` / `AtomicStore` / `AtomicCmpXchg` / `AtomicRMW` /
    `AtomicFence` (`MIRInstruction.h`, values 50–54; `Count` is now 55), with
    flags/names/portability in `MIRInstruction.cpp` and a `MIRDump` entry.
    Every ordering / scope / op / weak selector is a `Constant` operand so no
    emitter ever needs the AST.
  - `MIRLowerer::lower_atomic_intrinsic` recognizes the seven compiler-decl
    intrinsics **before** the captured-comptime-ref shortcut (the interpreter
    stores the call node itself as that ref's evaluated value, so the operands
    resolve through their own captures). Selectors must be compile-time
    constants — LLVM requires it — and come from `captured_ref_owner.refs[i].evaluated`
    (falling back to `comptime_value_eval_`), else it is a hard lowering error.
  - `CodegenMir.cpp` emits the LLVM atomics (incl. `AtomicRMWInst::BinOp`
    mapping, sync scopes, `cmpxchg`'s expected-value write-back, and the
    monotonic-fence upgrade / signal-fence no-op rules that mirror
    `LLVMBackendContext`).
  - `lang/libs/atomic/src/atomic.ch`: the LLVM branch of every helper now wraps
    the intrinsic call in `%runtime_value(...)`, mirroring the C branch's
    `%runtime_value(__chx__*_dispatch(...))` shape. That wrapper is what
    registers the operands as captured runtime values (a `CapturedComptimeVariable`),
    without which MIR cannot resolve the runtime pointer/value. Best-effort
    `atomic_fence` / `atomic_signal_fence` stay dropped exactly as on the C
    backend (the intrinsic is a statement in a void `comptime func`); they are a
    pre-existing gap on both backends, not a regression.
- **Other MIR/LLVM gaps fixed this session:**
  - `CodegenMir`: `MIRUnaryOp::Not` (logical `!`) emitted `CreateNot` on the
    operand's own type, so `!pointer` became `xor ptr %p, <13 x bfloat>`
    (garbage) and crashed SelectionDAG type legalization. It is now
    `x == 0` / `x == null` / `x == 0.0`; `BitNot` keeps the bitwise complement.
  - `MIRLowerer::intern_function` (+ the named-method path): a `public` /
    `protected` function now gets `MIRLinkage::External`, matching the C
    backend, instead of only `@extern` ones. Functions never visited by
    `mir_translate_after_declaration`'s declaration pass had internal linkage,
    producing cross-module undefined references.
  - `ASTProcessor::process_nodes` now descends into a top-level `if`'s taken
    scope (`IfStatement::computed_scope`), mirroring `MIRDeclEmitter::declare`.
    Previously a module lost every symbol declared inside a top-level arch/OS
    conditional — e.g. all of `std::concurrent`
    (`std/concurrency/threadpool.ch`), so any progam using
    `std::concurrent.spawn` failed to link.
  - `ASTProcessor`: with `mir_llvm_collect` set the C emission is skipped (the
    LLVM target emits IR from the collected MIR and never uses the C text).
    This also keeps the C emitter from having to understand LLVM-only ops.
  - `ASTProcessor.h`: `mir_llvm_collect` / `mir_llvm_finish` moved out of
    `#ifdef COMPILER_BUILD` — they are plain data consumed by the shared
    `implement_module`, and leaving them guarded broke the `TCCCompiler` target.

**Session 2 (this session) — first working LLVM binaries; four real bugs.**
The `Compiler` target now produces *runnable* LLVM binaries for real
applications (`import cstd` + `std`), matching the C backend byte-for-byte on
output. Everything below was found by building standalone probe modules under
`lang/compiled/` and diffing the two backends' behaviour.

- **`@extern` globals were emitted as *definitions*.** `Codegen::mir_declare_globals`
  created every MIR global with `Constant::getNullValue(t)` initializer, so
  cstd's `@extern var stdout : *mut FILE` became a defined BSS symbol that
  interposed libc's real `stdout` — every `printf` segfaulted
  (`__vfprintf_internal(s=0x0)`). External-linkage symbols are now emitted as
  uninitialized *declarations* (internal ones stay definitions), and the
  AST-literal global-initializer pass in `ASTCompiler.cpp` skips `@extern` vars.
  `nm -D` now shows `stdout@GLIBC_2.2.5` (a copy relocation) like the C build.
- **Capturing lambdas were undefined / mislaid-out on LLVM.**
  `MIRLowerer::materialize_capturing_lambda` built the closure *struct* only as
  C text, so the MIR record stayed an opaque empty struct (every
  `self->capture` GEP out of bounds), and the `_cap_destr` helper existed only as
  C text, so LLVM had no definition to link (`__chemda_cap_N_cap_destr`). The
  closure's runtime fields are now registered in the MIR type table, and the
  capture destructor is *also* built as MIR (one `Destroy` call per
  destructible capture) while the C text is kept for the C backend.
- **`DynamicCall` on a by-value fat pointer produced invalid IR.** The receiver
  is a `{ first, second }` struct passed by value; the emitter passed the SSA
  aggregate straight to `CreateStructGEP`, emitting an aggregate-base GEP
  (`getelementptr %__chemical_fat_pointer__, %__chemical_fat_pointer__ %v, …`).
  That is invalid IR and crashed instruction selection
  (`LLVM ERROR: Cannot select: bitcast`). The emitter now spills a value
  receiver into an entry-block alloca first.
- **The fat pointer's `second` field was named `secon`.**
  `MIRTypeBuilder::fat_pointer_type` hard-coded `fi.name_length = 5` for *both*
  `first` and `second`, so the vtable store could not resolve its field and was
  silently dropped (`have=,first,secon`). The length is now `strlen`.

With those fixed, dynamic dispatch (`dyn Interface`, `DynamicCall`, generated
`__chx_<iface>_vt_t` vtables + `ImplDecl` initializers), capturing lambdas /
`std::function`, struct methods, `@delete` ordering, nested struct member
access, `for`/`while`, globals and `printf` all match the C backend.

**Session verification (all re-run after the last edit):**

| Command | Result |
|---------|--------|
| `cmake --build --target Compiler` | builds |
| `cmake --build --target TCCCompiler` | builds |
| `./cmake-build-debug/MIRTests` | 1049 checks OK |
| `./scripts/test.sh --tcc` (clean per-module cache) | 2292 / 2292 passed, 0 failed |
| LLVM probe: `lang/compiled/mir_llvm_probe` (methods, dtor, `for`, printf) | output identical to the C build |
| LLVM probe: `lang/compiled/mirp3` (nested structs, method mutation, dtor) | output identical |
| LLVM probe: `lang/compiled/mirp4` (capturing lambdas / `std::function`) | output identical (`f=110 g=300 r=301`) |
| LLVM probe: `lang/compiled/mirp5` (`dyn` dispatch, vtable, dyn in struct) | output identical (`s=99 c=22 / pc=99 / p=22`) |
| `./scripts/test.sh --tcc --libs --no-build` | 719 total, 718 passed, 1 failed (the known flaky atomic CAS test) |

**Remaining gaps (LLVM target).** These are lowering/emission gaps, not branches
— each is a bug to fix:

- Synthesized default ctor / dtor (`make` / `delete` created by
  `MembersContainer::create_def_constructor` / `create_destructor`) and
  `comptime` constructors are *declared* but never emitted into MIR, so
  `test_report_TestLogmake`, `test_report_TestLogdelete`,
  `test_report_ConsoleReporterdelete` are undefined at link time
  (`lang/tests/libs/atomic`).
- CBI `EmbeddedNode` / `EmbeddedValue`, inline `asm`, and async coroutines are
  emitted from MIR-C only; the LLVM emitter does not lower them yet.
- Debug info is still not emitted from MIR (`--mode debug_complete` emits no
  DWARF from the MIR path).
- The AST-based LLVM lowering is now *dead code* behind the unconditional
  `mir_compile_module` early return in `ASTProcessor::compile_module`, but it is
  not deleted yet (§4 / checkpoint 8.9).

**Session 3 — the LLVM suite now runs most of the way; bug-fix sweep.**
The main suite previously aborted at test ~861; it now reaches test ~1473 before
the next crash. Every fix below is a MIR→LLVM emission bug (the C backend was
already correct), verified against the C build with `lang/compiled/` probes and
by the TCC suite staying at 2292/2292 (`MIRTests` 1049 checks OK).

- **Global initializers are now built as LLVM constants.** `mir_declare_globals`
  only knew null initializers and the old pass folded *scalars only*; arrays of
  cast elements, structs with defaulted/inherited fields, variant-shaped globals,
  string constants and comptime-constructed values were all silently zero (this
  was `(null)`-prefixed test output and the whole `phi xor lowering` block).
  `GlobalConstantLowerer` (`ASTCompiler.cpp`) now mirrors
  `MIRDeclEmitter::const_value_expr`: scalars, strings (owning a private
  `.mir.str.N`), arrays (zero-padded), structs (defaults + inherited bases),
  variants, casts, negations, comptime folds, and nested namespaces / taken
  comptime branches. An unsized `[0 x i8]` global (`const x : char[] = "..."`)
  is rebuilt with the extent inferred from the literal.
- **Integer promotions are narrowed back.** MIR keeps the C promotion semantics
  and relies on the assignment to truncate; LLVM needs an explicit `trunc`, so
  `(i8 100) + (i8 100) == -56i8` and every `i16` overflow case failed. `Binary`
  and `Unary` now truncate their promoted-width result to the MIR value's type.
- **Casts to pointers sign-extend.** `(void*)(int)-1` must be all-ones; LLVM's
  `CreateIntToPtr` only zero-extends. `Cast` now extends the integer to pointer
  width with the *source* signedness first.
- **Casts to an `Opaque` type (a static interface) keep the pointer.** `mir_type`
  spells `Opaque` `i8`, so `value as StaticInterface` became
  `ptrtoint ptr %p to i8` — truncating a 64-bit address to 8 bits. The cast now
  preserves the pointer (the erased-self ABI is a pointer).
- **`Drop`/`Destroy` mirror the C emitter.** A pointer-typed (aggregate-param)
  place passes the *loaded* pointer as `&self`, not the address of its own stack
  slot (a struct holding a capturing lambda passed to a lambda clobbered its
  `lamb` field); an array place destroys each element **last-to-first**.

**Still open on the LLVM target (next checkpoints):** the main suite aborts at
`main_test_native_generic_fn_reference` (~test 1927) on a null generic-function
call; `string arrays over sized`, `inherited default initialized structs`,
`getting dynamic object from function return`, and `sizeof/align of struct SOT6`
still fail; synthesized `make`/`delete` MIR defects; CBI `EmbeddedNode`, inline
`asm`, async, and debug info are not lowered from MIR yet; the AST lowering is
still undeleted.

**Session 3 (cont.) — dynamic dispatch, zeroing and aggregate `memcpy`.**
After the first sweep the suite reached ~1890:

- **Primitive-impl vtables were never emitted.** `mir_emit_llvm_module`'s ImplDecl
  loop required a `MembersContainer`, so `impl Interface for int` produced no
  `__chx_<iface>_<type>_vt` global, the `dyn<I>` fat pointer's `second` slot had
  nothing to point at and the store was dropped altogether (`dyn_prim_impl_give`
  called a garbage vtable → SIGSEGV). Primitive impls now build the vtable from
  the impl's own methods, mirroring `MIRDeclEmitter::create_vtable_primitive`.
- **`zero_init(place)` carries only a place operand.** The LLVM `MemSet` required
  byte+size, so every `zeroed<T>()` / array-literal / partial-literal zeroing was
  dropped. It now zeroes `sizeof(place)` bytes, like the C emitter.
- **`memcpy p1, ptr` carries no size operand.** The C emitter defaults to
  `sizeof(dest)`; the LLVM `MemCpy` required the size and silently dropped the
  instruction — so `var x = *j` (an aggregate copy through a reference/pointer)
  never copied, and reading `x.<field>` read uninitialized storage. The size now
  defaults to the destination place's alloc size.

Only 5 main-suite failures remain (was 18). The C path is unaffected
(MIRTests 1049 checks OK; `TCCCompiler` builds unchanged).

**Session 3 (cont. 2) — the main suite now runs to completion.**
It no longer aborts mid-run: `Total 2294 Passed 2275 Failed 19`. Three more
MIR→LLVM emission bugs, all invisible on the C path (one translation unit hides
criss-cross-module references):

- **A global initializer that is a function reference stayed null.**
  `GlobalConstantLowerer::build_impl` had no case for a function used as a value
  (`var f : (x : int) => int = generic_identity<int>`), so the global kept the
  `null` initializer from `mir_declare_globals` and calling through it segfaulted
  (`main_test_native_generic_fn_reference`). `Identifier` /
  `GenericInstIdentifier` and the non-call `AccessChain` leaf now resolve to the
  concrete `FunctionDecl` and its pre-declared LLVM function — mirroring
  `MIRDeclEmitter::const_value_expr`'s `FunctionDecl`-leaf case.
- **Cross-module vtable references were dropped.** An interface vtable is emitted
  in the module that hosts the `impl` with (C) `static` linkage, which the LLVM
  backend spelled `internal`; a `dyn<I>` built in *another* module then had no
  symbol to reference, so `GlobalAddr` produced no value and the store into the
  fat pointer's `second` slot was silently dropped (calling through a garbage
  vtable → SIGSEGV in `main_dyn_sum_pub_norm_inter`). Vtable globals are now
  `external` (their mangled names embed both the interface and the impl type, so
  they stay program-unique) and `CodegenMir`'s `GlobalAddr` emits an external
  declaration for a named global it does not own.

The remaining 19 failures are: `string arrays over sized`, `inherited default
initialized structs`, `getting dynamic object from function return` (2),
`sizeof/align of struct SOT6`, and 14 `std::string`/`string_view` `to_i32`-family
conversions (`parse_int` returns garbage on the LLVM target only).

**Session 4 — the main suite is green on the LLVM target.**
`./scripts/test.sh --llvm` → **2298 passed / 0 failed** (the five remaining
failures are fixed). The `to_i32`-family conversions had already been fixed
off-session; the last five were:

- **An empty aggregate was padded with an `i8`.** `Codegen::mir_type` appended
  `elems.push_back(i8)` when a struct/union/variant had no fields, so
  `sizeof(struct SOT6 {})` was `1` while the interpreter's comptime `sizeof` and
  TinyCC both say `0` (`struct S { }`). The padding also shifted every field of
  any struct *containing* an empty aggregate away from the C layout. Removed:
  an empty aggregate now lowers to an empty LLVM struct (zero-sized), matching
  the C emitter and the interpreter.
- **Aggregate `alloca`s were not zeroed.** The C emitter zero-initializes every
  `Alloca` of a struct/union/variant/array type ("so unset members are
  well-defined"); the LLVM emitter left them uninitialized and relied on the
  occasional explicit `MemSet`. A partial aggregate initializer such as
  `var s : [10]char = "hello"` only `memcpy`s the 6 literal bytes, so
  `s[6]`–`s[9]` held garbage. The LLVM `Alloca` case now emits the same
  `memset` at the same program point (so a loop-body alloca is re-zeroed per
  iteration, exactly like the C build).
- **The dotted field path had only one level.** An inherited member is lowered
  to a full dotted path (`Base.Mid.Leaf.field`, see
  `MIRLowerer::inherited_field_path`), which the C emitter can splice straight
  into a C access chain. `CodegenMir`'s `field_path` split on the *first* dot
  only, so a two-or-more-level base chain resolved to nothing and the field
  read became `poison` (`test_inherited_default_initialized_structs`, where
  `f.d` worked and `f.a`/`f.b`/`f.c` did not). It now walks every segment — the
  same shape covers a variant's `Case.param` payload path.
- **A `dyn<T>` built from a by-value parameter dangled.** The fat pointer's
  `first` slot took the address of the parameter's *copy*, a local of the
  callee, so `func f(p : S) : dyn I { return dyn<I>(p) }` handed the caller a
  pointer into a dead frame — the next call (the vtable method itself) reused
  that slot and clobbered it. The C backend only appeared to work because
  TinyCC happened not to reuse the bytes. `MIRLowerer` now records the hidden
  argument pointer of every by-value aggregate parameter
  (`aggregate_param_ptrs_`) and `DynamicValue` lowering uses it, so the fat
  pointer refers to the *caller's* object. That is what the interpreter does
  (it shares the argument value) and what the documented "aggregate params are
  pointer-typed places" model says; regression test:
  `lang/tests/common/src/decls/interfaces/dynamic.ch` →
  *dynamic object built from a by-value parameter survives the callee frame*
  (fails with `0` instead of `88` without the fix on the C backend too).

Verification after the fixes (clean per-module cache for each run):

| Command | Result |
|---------|--------|
| `./scripts/test.sh --llvm` | 2298 / 2298 |
| `./scripts/test.sh --tcc` | 2300 / 2300 |
| `./scripts/test.sh --tcc --interpret` | 1890 / 1890 |
| `./cmake-build-debug/MIRTests` | 1049 checks OK |

The C path improvements are incidental but real: the zeroed-alloca and
`dyn`-from-parameter fixes are in shared MIR lowering/`CodegenMir`, so the C
backend gets them too.

### 2.1 What is done

- **MIR owns all C emission.** Every function body, static-interface impl,
  CBI `EmbeddedNode` replacement, static-interface stub, and async/coroutine
  lowering goes through MIR. The legacy 2c visitor is deleted.
- **All TCC suites are green** (verified at baseline `8001ecde3`):
  - `--tcc` → 2292 passed / 0 failed
  - `--tcc --interpret` → 1883 / 0 failed
  - `--tcc --libs` → 718/719 (one flaky atomic CAS test; passes in isolation)
  - `--tcc --async` → 53 / 0
  - `--tcc --plugins` → 1296 / 0
  - `--tcc --process` → 127 / 0
  - `--tcc --server` → 8/9 (known flaky double-free in the HTTP parser)
  - `MIRTests` → 1049 checks OK
- **Core-language bugs fixed just before this migration** (do not re-introduce;
  regression tests exist):
  - Destruction order was forward; now reverse (scope, array, interpreter).
    Tests: `lang/tests/common/src/destructors/order.ch`.
  - Whole-array assignment emitted invalid C; now `memcpy`.
    Tests: `lang/tests/src/basic/array_assignment.ch`.
  - Pointer-to-array (`*mut [N]T`) was decayed to `T**`; now `T (*)[N]`.
    Tests: `lang/tests/src/basic/pointer_to_array.ch`.

### 2.2 What is NOT done (the migration target)

- **LLVM still lowers from the AST.** The `Compiler` target is built with
  `COMPILER_BUILD` and drives `Codegen` + per-AST-node `llvm_*` methods.
- **MIR carries no source locations.** `MIRInstruction::source_index` and
  `MIRFunction::sources` exist but the lowerer never fills them
  (`source_index` is always `0xFFFF`). The MIR C emitter emits **no `#line`
  directives**, so C debug line info is also currently lost.
- **Aggregate layout is not filled.** `MIRTypeRecord.size`/field data are not
  populated for aggregates; `MIRTypeRecord` has `data_offset`/`data_count` for
  field/element type ids but no field-offset table. This is the documented
  blocker before LLVM lowering.
- **No MIR→LLVM type lowering, ABI table, or emitter.** There is no
  `MIRABIEntryTable`; call ABI is implicit in the C emitter's spelling.
- **Async is MIR-lowered only for C.** The LLVM path uses `LLVMCoroutine.cpp`
  with `llvm.coro.*` intrinsics.

### 2.3 Health of the LLVM gate (read before trusting it)

`./scripts/test.sh --llvm` (the main suite) is **green**: 2298 / 2298 as of
session 4. The earlier `fatal: llvm_pointer called on bare Value` abort is gone.

**`--llvm --libs` no longer aborts in the compiler** (re-checked 2026-10-13,
session 11): the full lib test exe builds cleanly — build the LLVM compiler and
run the libs suite, or reproduce directly with
`cmake-build-debug/Compiler lang/tests/build.lab -o /tmp/libs_llvm.exe --mode
debug_quick --no-cache --arg-test-libs` → exit `0`, all 30 modules emitted. The
"`RUNTIME ERROR: invalid memory access` while building the lib test exe" recorded
in the session-4 note below no longer reproduces; the other non-main LLVM suites
have still not all been re-checked.

- ⚠️ A failing `--llvm <suite>` run is *not* proof of a codegen bug: check
  whether the compiler itself crashed before the test exe ran (the suite exits
  with the compiler's crash reporter, and no `Total …` line is printed).
- For a suite whose first failure is in the compiler, use the standalone module
  workflow (§9.3) plus `--out-ll-all`/`llvm-dwarfdump` to isolate it.

> **Historical (session 4) state, kept for context:** `./scripts/test.sh --llvm
> --libs` used to abort in the compiler while building the lib test exe (after
> `…/lang/libs/json/src/encode.ch`, with `RUNTIME ERROR: invalid memory
> access`), emitting nothing.

---

## 3. Architecture map

### 3.1 MIR path (C) — the model to follow

| File | Role |
|------|------|
| `compiler/mir/MIRTypes.h` | id types, `MIROperandKind`, `MIRStorageClass`, flags |
| `compiler/mir/MIRInstruction.h/.cpp` | `MIROpcode`, `MIROperand`, `MIRInstruction`, flags |
| `compiler/mir/MIRFunction.h` | blocks, dense value/place tables, cleanup scopes, `sources` |
| `compiler/mir/MIRTypeTable.h` | canonical `MIRTypeRecord` interning + name/data pools |
| `compiler/mir/MIRTypeBuilder.h/.cpp` | `BaseType*` → `TypeId` (only core file including AST types) |
| `compiler/mir/MIRBuilder.h/.cpp` | typed construction API (places, fields, indices, calls, lifetime) |
| `compiler/mir/MIRLowerer.h/.cpp` | AST → MIR lowering (includes AST headers) |
| `compiler/mir/MIREmitter.h/.cpp` | MIR → C (AST-free) |
| `compiler/mir/MIRDeclEmitter.h/.cpp` | MIR → C declarations/prototypes/globals |
| `compiler/mir/MIRDump.h/.cpp`, `MIRVerifier.h/.cpp` | debug dump + structural checks |
| `compiler/mir/MIRModule.h` | symbols, constants, types, per-module tables |
| `compiler/mir/tests/mir_tests.cpp` | standalone `MIRTests` (no LLVM link) |

Integration driver: `compiler/ASTProcessor.cpp` (`mir_translate_after_declaration`,
`mir_declare_module`, callbacks) and `compiler/lab/LabBuildCompiler.cpp`.

MIR gets the mangler/interpreter through callbacks (`set_mangler`,
`set_comptime_eval`, `set_embedded_repl`, `set_async_normalize`, …) so
`compiler/mir/*` stays AST-free except `MIRTypeBuilder`/`MIRLowerer`.

### 3.2 LLVM path (AST) — to be deleted / replaced

| File | Role | Fate |
|------|------|------|
| `compiler/Codegen.h/.cpp` | AST codegen driver (`declare_nodes`, `compile_nodes`, scopes, destruct jobs, DI) | Delete/replace; salvage target-machine + module plumbing into the MIR LLVM driver |
| `compiler/backend/LLVM.cpp` | all AST node `llvm_value`/`llvm_pointer`/`llvm_type`/`code_gen` | **Delete** |
| `compiler/backend/LLVMCoroutine.cpp/.h` | AST async coroutine lowering (`llvm.coro.*`) | Rework to consume MIR, or delete and lower coroutines from MIR |
| `compiler/backend/LLVMGen.h/.cpp` | thin alloca/store/GEP helpers | Keep as helper |
| `compiler/backend/DebugInfoBuilder.h/.cpp` | DI compile unit / DI types / DI locations from AST | **Rework to consume MIR metadata** (§5.1) |
| `compiler/backend/LLVMBackendContext.h` | `BackendContext` for `get_backend_name()=="LLVM"` | Keep (thin) |
| `compiler/backend/CLANG.cpp`, `clang_*`, `llvm-ar.cpp`, `chem_clang.cpp` | embedded clang/LLD driver, linking | **Keep** (not lowering) |
| `compiler/ASTCompiler.cpp` | `ASTProcessor::code_gen_declare/compile` for the AST driver | Delete/replace |
| `#ifdef COMPILER_BUILD` blocks in `ast/**` | ~230 sites across ~70 files: `llvm_value`, `llvm_pointer`, `llvm_type`, `code_gen*`, `llvm_destruct`, `llvm_param_type`, `add_member_index`, … | **Delete** once MIR→LLVM covers them |

`Codegen.h` also defines AST-facing structures (`Destructible`, coroutine
contexts, `current_loop_exit`, `loop_destr_job_begin_index`, etc.). Those are
AST-lowering state and go with the deletion; MIR already models cleanup, drop
flags, and async state.

### 3.3 Build targets & macros

| Target | Macro(s) | Links | Backend |
|--------|----------|-------|---------|
| `Compiler` | `COMPILER_BUILD CLANG_LIBS LLD_LIBS` | LLVM/Clang/LLD + TinyCC | **LLVM** (to become MIR→LLVM) |
| `TCCCompiler` | `TCC_BUILD` | TinyCC | MIR→C |
| `ChemicalLsp` | `LSP_BUILD` | lsp-framework | analysis only |
| `MIRTests` | — | none (no LLVM) | MIR core only |

- `COMMON_SOURCES` includes the MIR core `.cpp`s (so both compilers build them).
- `COMPILER_SOURCES` (LLVM-only) lists `Codegen.cpp`, `backend/LLVM.cpp`,
  `LLVMCoroutine.cpp`, `DebugInfoBuilder.cpp`, `LLVMGen.cpp`, `CLANG.cpp`, etc.
  `CMakeLists.txt` around lines 741–784.

**Constraint:** the `TCCCompiler` binary must stay **under 4 MB** and must not
link LLVM. MIR headers must not include LLVM headers; the MIR→LLVM emitter must
be a separate translation unit compiled only for the `Compiler` target (the same
way `MIREmitter` is AST-free and shared, while LLVM code is `COMPILER_BUILD`-only).

### 3.4 MIR opcodes (what the LLVM emitter must handle)

`MIRInstruction.h` `MIROpcode` (50 values): constants (`ConstInt/Float/Double/
Bool/Null/String`), storage (`Alloca`, `GlobalAddr`, `FunctionAddr`, `Param`,
`Load`, `Store`, `AddressOf`, `Gep`, `FieldAddr`, `IndexAddr`), lifetime
(`Init`, `CopyInit`, `MoveInit`, `Assign`, `Copy`, `Move`, `Drop`, `Destroy`,
`SetDrop`, `MemCpy`, `MemSet`), arithmetic (`Unary`, `Binary`, `Compare`, `Cast`,
`Select`, `SizeOf`, `AlignOf`, `OffsetOf`), calls (`Call`, `CallIndirect`),
terminators (`Br`, `CondBr`, `Switch`, `Return`, `Unreachable`, `Throw`), async
(`AsyncAwait`, `AsyncFinish`, `AsyncRampFinish`, `AsyncFrameAlloc`), `InlineAsm`,
`DynamicCall`.

Design reference: `mir-design.md` §10.1 (SSA values→LLVM SSA, places→alloca,
load/store, GEP, calls with ABI, terminators, PHI nodes, DI locations).

---

## 4. What "delete the LLVM lowering" means (scope)

1. Route the `Compiler` job's codegen through MIR: build `MIRModule`/`MIRFunction`
   in `ASTProcessor` exactly as the C path does, then call the MIR→LLVM emitter
   instead of `gen.compile_nodes(...)`.
2. Delete `backend/LLVM.cpp` and the per-AST-node `llvm_*`/`code_gen` methods
   (remove the `COMPILER_BUILD` blocks in `ast/**`). This is the large, mechanical
   part; do it only after the MIR→LLVM emitter covers those constructs, and in
   batches so the build stays compilable.
3. Delete/replace `Codegen`'s AST-facing state; keep only what the MIR LLVM
   driver needs (LLVM context/module/IRBuilder, target machine, DI builder,
   linking).
4. Rework `DebugInfoBuilder` to take MIR data (§5.1).
5. Delete `ASTCompiler.cpp`'s AST `code_gen_*` helpers.
6. **Do not** delete the embedded clang/LLD/ar driver or the C→Chemical
   `CTranslator` (reverse direction).

---

## 5. Hard requirements

### 5.1 Debug information must be preserved (explicit requirement)

Today the AST-based LLVM path builds DWARF from AST nodes via
`DebugInfoBuilder` (`create_struct_type(StructDefinition*)`,
`create(FunctionTypeBody*, llvm::Function*)`, `instr(llvm::Instruction*, SourceLocation)`).
**After the migration, debug info must be built from MIR.** That requires:

1. **Thread source locations into MIR.**
   - `MIRLowerer` must populate `MIRFunction::sources` (`MIRSourceLoc{file,line,
     column}`) and set `MIRInstruction::source_index` for each instruction it
     emits (at minimum for statements/expressions/calls/branches).
   - `MIRBuilder` needs a location parameter/current-location setter so lowering
     can attach locations without touching every call site by hand.
   - The C emitter may then emit `#line` directives (restoring the legacy
     `line_directives` behavior) — this is a bonus, but LLVM DI is mandatory.
2. **MIR→LLVM DI construction.** The DI builder must accept MIR type/function/
   location metadata:
   - `MIRTypeRecord` → `llvm::DIType` (needs aggregate layout + field names).
   - `MIRFunction` (symbol name, source location, parameter types) →
     `DISubprogram`/`DICompileUnit`.
   - `MIRSourceLoc` → `llvm::DILocation`, attached to every emitted instruction.
   - MIR must retain the **source declaration id** (`MIRTypeRecord.decl`) and
     enough file/line info to point DWARF at the original `.ch` file, not at
     generated C.
3. **Verification:** when `--mode debug_complete` / `-g` is set, the emitted
   `llvm_ir.ll` must contain `!DICompileUnit`, `!DISubprogram`, `!DILocation`,
   `!DILexicalBlock`, `!DIBasicType`, `!DICompositeType`, and
   `llvm.dbg.declare`/`llvm.dbg.value` intrinsics. Validate with:
   ```bash
   cmake-build-debug/Compiler <mod> -o out.exe --mode debug_complete --assertions \
       --out-ll-all --build-dir <dir>
   grep -E "DICompileUnit|DISubprogram|DILocation|dbg.declare" <dir>/modules/*/llvm_ir.ll
   llvm-dwarfdump --debug-line out.exe | head    # or readelf --debug-dump=decodedline
   ```
   A release-mode (`debug_quick` / no `-g`) build must emit **no** DI metadata.

**Policy:** a change that drops debug info is a regression even if tests pass.
Add a debug-info smoke check to the verification checklist for every Stage 8
checkpoint.

### 5.2 No flags / no fallback / no capability analysis

Same as `mir-implementation-plan.md` §0. No `--use-mir`/`--no-llvm`, no
`if (mir_supports(x))`, no per-function legacy LLVM fallback. If MIR cannot
lower a construct, that is a **bug to fix**, not a branch to add. A lowering
failure must be a diagnostic, not a fallback.

### 5.3 Performance / size

- MIR construction + C emission ≤ direct 2c translation time for `debug_quick`
  (see plan §2.3). LLVM emission may be serial per module.
- `TCCCompiler` stays < 4 MB and LLVM-free.
- `MIRTests` must stay green (it does not link LLVM; keep it that way).

---

## 6. Prerequisites (blockers to clear first)

1. **Aggregate layout in MIR types.** Fill `MIRTypeRecord` for structs/unions/
   variants: `size`, `alignment`, and a field table (name, offset, type id,
   destructor flag). This is needed by both the LLVM type lowering and DI.
   (`compiler/mir/README.md` "Aggregate layout milestone".)
2. **Source locations in MIR** (§5.1). Needed for DI; also restores C `#line`.
3. **MIR→LLVM type lowering.** `TypeId`/`MIRTypeRecord` → `llvm::Type*`
   (ints by width/signedness, bool, float/double, pointers, arrays, structs by
   layout, function types, `__chemical_fat_pointer__`, variants-as-structs).
4. **Call ABI.** Decide and encode the calling convention the C backend already
   uses: sret for aggregate returns, aggregate params by hidden pointer, the
   capturing-function ABI `(void* this, sret?, params)`, variadic tails. The
   design names this `MIRABIEntryTable`; it does not exist yet.
5. **Async/coroutine lowering for LLVM.** Either (a) lower `AsyncAwait` etc. to
   `llvm.coro.*` intrinsics, or (b) keep the existing frame/state-machine shape
   and emit it as normal LLVM functions from MIR. Whichever is chosen must match
   the C backend's observable behavior (`lang/tests/async`).
6. **MIRVerifier / MIRDump coverage** for the new metadata (layout, sources).

---

## 7. Phased plan (one checkpoint per AI session)

Each phase must end with the relevant suites green (or a precise explanation of
the pre-existing LLVM-suite breakage, §2.3) and `MIRTests` green.

- **8.0 — Inventory & kill-switch plan (docs only).** Enumerate every
  `COMPILER_BUILD` codegen site and map it to the MIR construct that will replace
  it. Produce the deletion order. No code changes.
- **8.1 — Aggregate layout.** Fill `MIRTypeRecord` layout + field tables in
  `MIRTypeBuilder`. Add `MIRTests` for struct/union/variant/array sizes and
  offsets (compare against `sizeof`/`offsetof` in C tests). C backend unchanged.
- **8.2 — Source locations.** Add a location API to `MIRBuilder`, populate
  `sources`/`source_index` in `MIRLowerer`, emit `#line` in the C emitter behind
  the existing debug option. Add a test that debug mode produces `#line` and
  release mode does not. `MIRTests` for source table round-tripping.
- **8.3 — MIR→LLVM type + module scaffolding.** New `MIRLLVMEmitter.{h,cpp}`
  (`COMPILER_BUILD` only): type lowering, globals, function prototypes, DI
  compile unit/subprogram from MIR. Keep the AST LLVM path alive behind the
  build for now **only** as a differential oracle during development — but do
  **not** ship a runtime flag (final switch is unconditional).
- **8.4 — MIR→LLVM function bodies (scalars/control flow/calls).** Lower all
  non-aggregate opcodes, terminators, calls, `Gep`/`FieldAddr`/`IndexAddr`,
  `Select`, casts, `SizeOf`/`AlignOf`/`OffsetOf`. Attach `DILocation`s.
  Differential-test MIR→LLVM vs the current LLVM output on the main suite.
- **8.5 — Aggregates & lifetime in LLVM.** Struct/array/variant values, sret,
  aggregate params, `CopyInit`/`MoveInit`/`Drop`/`Destroy`/`SetDrop`, drop flags,
  arrays. Verify destructor ordering tests pass on LLVM.
- **8.6 — Interfaces / dynamic dispatch / capturing functions / lambdas.**
  `DynamicCall`, fat pointers, CBI `EmbeddedNode`/`EmbeddedValue` replacements.
- **8.7 — Async/coroutine lowering in LLVM.** Match `--async` behavior.
- **8.8 — Switch the `Compiler` job to MIR→LLVM unconditionally.** Delete the
  AST driver wiring in `LabBuildCompiler`/`ASTProcessor`.
- **8.9 — Delete the AST LLVM lowering.** Remove `backend/LLVM.cpp`,
  `ASTCompiler.cpp`, `Codegen` AST state, and the `#ifdef COMPILER_BUILD`
  codegen methods in `ast/**`. Fix the build, keep `MIRTests` and TCC green.
- **8.10 — Debug-info parity & sign-off.** Verify DWARF for a real app, add a
  debug-info smoke test/CI check, update `compiler/mir/README.md` and this doc.

---

## 8. Bug & test policy (core-language regressions)

While doing any phase, if you find a bug in MIR (lowering, builder, emitter,
type mapping, common/interpreter code) that would affect the **C backend too**
and concerns a **core language feature**:

1. **Fix it.**
2. **Add a regression test** so it cannot come back. Placement rules (from
   `.agents/skills/testing/SKILL.md`):
   - **Pure language, no `std`/native interop, works in the interpreter** →
     `lang/tests/common/src/…`, registered from `run_common_tests()` in
     `lang/tests/common/src/main.ch`. This runs on the C backend, the LLVM
     backend, **and** the interpreter.
   - **Needs `std`/`cstd`/`fs`/`atomic`/threads or cross-module deps, or has a
     known interpreter gap, or is backend-specific** →
     `lang/tests/src/…`, registered from `run_executable_tests()` in
     `lang/tests/src/tests.ch`. Use `comptime if(intrinsics::get_backend_name()
     != "LLVM")` (or `== "C"`) to guard a test to the C backend when the LLVM
     path is known-broken.
   - **Pointer arithmetic** (interpreter does not model it) → `src/` or
     `native_common/`, not `common/`.
3. Prefer an **inline `test("name", () => bool)`** call. Keep each test to one
   behavior. Model examples: `common/src/destructors/order.ch`,
   `src/basic/pointer_to_array.ch`.
4. If the bug is a **negative/compile-error** case, add it to the negative suite
   (`lang/tests/negative/src/main.ch`).

Do **not** encode a bug as an expected behavior just to keep a suite green.

---

## 9. Verification & commands

### 9.1 Build

```bash
# MIR→C compiler (TCCCompiler) + MIR unit tests
cmake --build cmake-build-debug --target TCCCompiler MIRTests -j 8
./cmake-build-debug/MIRTests                 # expect: OK (1049+ checks)

# LLVM compiler (Compiler target; links LLVM/Clang/LLD)
./scripts/build.sh --llvm
# or: cmake --build cmake-build-debug --target Compiler -j 8
```

### 9.2 Test suites

> ⚠️ The per-module C is cached. After a compiler change you MUST clear
> `lang/tests/build/chemical-tests.dir` **and** `lang/tests/build/lab` (and
> `lang/tests/build/*.dir` for plugin/CBI changes), or you will run stale code.

```bash
rm -rf lang/tests/build/chemical-tests.dir lang/tests/build/lab

./scripts/test.sh --tcc                    # main (C/MIR)
./scripts/test.sh --tcc --interpret        # interpreter
./scripts/test.sh --tcc --libs             # libraries
./scripts/test.sh --tcc --plugins          # CBI plugins
./scripts/test.sh --tcc --async            # coroutines
./scripts/test.sh --tcc --process
./scripts/test.sh --tcc --server
./scripts/test.sh --tcc --regexp
./scripts/test.sh --llvm                   # LLVM backend (see §2.3 caveat)
```

**Never run `./scripts/test.sh --all`** (agents): it runs every suite including
the slow `tls` suite. Run only the suites relevant to the change.

### 9.3 Standalone module (fast iteration, full library access)

```bash
mkdir -p lang/compiled/my_test/src
cat > lang/compiled/my_test/chemical.mod <<'EOF'
application my_test
source "src"
import cstd
import std
EOF
# ... write src/main.ch ...

# LLVM IR + debug info:
cmake-build-debug/Compiler lang/compiled/my_test/chemical.mod \
    -o lang/compiled/my_test/main.exe \
    --mode debug_complete --assertions \
    --out-ll-all --build-dir lang/compiled/my_test/build/
# IR: lang/compiled/my_test/build/modules/<name>/llvm_ir.ll

# C translation:
cmake-build-debug/TCCCompiler lang/compiled/my_test/chemical.mod \
    -o lang/compiled/my_test/main.exe --emit-c
# C: lang/compiled/my_test/build/modules/<name>/partial.2c.c

# Run / debug:
./lang/compiled/my_test/main.exe
gdb -batch -ex run -ex bt --args ./lang/compiled/my_test/main.exe
```

`lang/compiled/` is git-ignored — use it freely for probes.

### 9.4 Debug-info check (mandatory per checkpoint)

```bash
grep -E "DICompileUnit|DISubprogram|DILocation|DIBasicType|DICompositeType|dbg.declare" \
    lang/compiled/my_test/build/modules/*/llvm_ir.ll | head
readelf --debug-dump=decodedline lang/compiled/my_test/main.exe | head   # or llvm-dwarfdump
```

### 9.5 Useful flags

`--emit-c`, `--keepc`, `--out-ll-all`, `--debug-ir`, `--assertions`,
`-fno-unwind-tables`, `--mode debug_quick|debug_complete`, `-g`,
`--minify-c`, `--use-c` (translate to C + embedded clang).

---

## 10. Gotchas (carry-over from the MIR work)

- **Per-module C cache**: clear `lang/tests/build/chemical-tests.dir` and
  `lang/tests/build/lab` after a compiler change. `--no-cache` alone does **not**
  invalidate per-module C.
- **CBI plugin object cache** is not invalidated when the compiler changes;
  clear `lang/tests/build/*.dir` before judging a plugin change.
- **Enum sync rule**: adding a `TokenType`/`ASTNodeKind` value requires the
  matching `.ch` binding (`lang/libs/compiler/src/…`) or CBI plugins SIGSEGV.
- **MIR type spelling must match the prototypes** emitted by the declaration
  pass (`declare_module`). A mismatch is a TinyCC "incompatible types for
  redefinition" error.
- **`value id 0 == MIR_NULL`**; don't use `result_or_place == MIR_NULL` as
  "no result".
- **Aggregate params are pointer-typed places**; decide deref with
  `needs_aggregate_path(ast_type)`.
- **Destruction order is reverse** for scopes/functions/arrays; struct members
  are declaration order. Use `emit_drops_from(mark)`.
- **Arrays are not assignable in C**: whole-array destinations must `memcpy`.
- **`*mut [N]T` is pointer-to-array** (`T (*)[N]`), not `T**`.
- `memset` conflicts with cstd's `extern void* memset(...)` — emit an inline
  zero loop in generated C.
- Full gotcha list: `compiler/mir/README.md` → "Gotchas learned".

---

## 11. Open questions / risks

1. **Is the `--llvm` suite broken before the MIR work?** (§2.3) Confirm against
   `main`; if yes, the migration's gate is "standalone modules + TCC suites +
   MIRTests", and fixing the LLVM suite is part of Stage 8.
2. **Coroutine strategy for LLVM** (intrinsics vs state-machine from MIR) — decide
   in 8.7; must match `--async` behavior.
3. **ABI encoding** — the C backend's implicit ABI (sret, by-pointer aggregates,
   fat pointers, variadics) must be made explicit for LLVM.
4. **Debug-info fidelity** — DWARF should reference original `.ch` files and
   line/column ranges; MIR must retain the file identity, not just a line number.
5. **Performance of DI in `debug_complete`** — ensure DI construction does not
   regress `debug_quick` (no DI there).

---

## 12. Appendix — quick file index

- MIR: `compiler/mir/` (see §3.1). Contract: `lang/docs/mir-design.md`,
  `lang/docs/mir-implementation-plan.md`. Living status: `compiler/mir/README.md`.
- LLVM (to delete/rework): `compiler/Codegen.{h,cpp}`,
  `compiler/backend/LLVM.cpp`, `compiler/backend/LLVMCoroutine.{h,cpp}`,
  `compiler/backend/DebugInfoBuilder.{h,cpp}`, `compiler/ASTCompiler.cpp`,
  `#ifdef COMPILER_BUILD` code in `ast/**`.
- Keep: `compiler/backend/CLANG.cpp`, `clang_*`, `llvm-ar.cpp`, `chem_clang.cpp`,
  `compiler/backend/LLVMGen.{h,cpp}`, `compiler/ctranslator/CTranslator.{h,cpp}`.
- Driver: `compiler/lab/LabBuildCompiler.cpp` (LLVM job ~lines 2080–2180),
  `compiler/ASTProcessor.cpp` (MIR wiring ~lines 1676–2160).
- Tests: `lang/tests/common/`, `lang/tests/src/`, `lang/tests/async/`,
  `lang/tests/negative/`, `compiler/mir/tests/mir_tests.cpp`.
