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

- **Debug info is complete (session 15, 2026-10-14).** Checkpoint **8.10** is
  done: MIR carries the source name and lexical scope of every named place and
  the extent of every source scope, and `CodegenMir` emits a `DILocalVariable` +
  `dbg.declare` record per local and a `DILexicalBlock` per scope, so the
  executable's DWARF has 1084 `DW_TAG_variable` and 61 `DW_TAG_lexical_block` on
  the probe where it previously had none. `debug_quick` still emits no DI at all.
  New smoke check: `./scripts/mir-debug-info-check.sh`. Details: **§2.6**.
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

> **Status (2026-10-13, session 12): most of this list is now DONE.** The AST
> LLVM lowering is *unreachable* — `ASTProcessor::compile_module` returns
> `mir_compile_module(gen, module)` unconditionally, so the entire
> `Codegen`/`backend/LLVM.cpp` AST path is dead code for Chemical modules. MIR
> now emits LLVM IR including debug info (see §2.4). What remains is the
> mechanical deletion plus one reachable legacy use (the C-header declaration
> path).

- ~~**LLVM still lowers from the AST.**~~ **DONE.** `compile_module` always goes
  through `mir_compile_module`; the AST `Codegen`/`llvm_*` lowering is dead but
  not yet deleted.
- ~~**MIR carries no source locations.**~~ **DONE (session 12).** The lowerer
  fills `MIRFunction::sources`/`decl_loc` and `MIRInstruction::source_index`
  through `MIRBuilder::set_source` + the `set_source_loc_provider` callback, and
  `CodegenMir` emits `DICompileUnit`/`DISubprogram`/`DILocation`/DI types.
  (The MIR **C** emitter still emits no `#line` directives — a bonus, not a
  requirement.)
- ~~**Aggregate layout is not filled.**~~ **DONE (session 10, action A1).**
  `compiler/mir/MIRLayout.h` fills `size`/`alignment`/field offsets.
- ~~**No MIR→LLVM type lowering, ABI table, or emitter.**~~ **DONE.**
  `compiler/CodegenMir.cpp` lowers MIR to LLVM; the C ABI is mirrored.
- ~~**Async is MIR-lowered only for C.**~~ **DONE.** Async lowers to a MIR state
  machine emitted by both backends; `LLVMCoroutine.cpp` is dead.
- ~~**Still open for full §5.1 DI parity:** `llvm.dbg.declare`/`dbg.value` for
  locals and `DILexicalBlock`s.~~ **DONE (session 15).** MIR now carries the
  source name of every named place (`MIRPlaceDef::debug_name` + the lexical scope
  it was declared in, `debug_scope`) and a lexical-scope table
  (`MIRLexicalScope`), and `CodegenMir` emits a `DILocalVariable` +
  `dbg.declare` record per local and a `DILexicalBlock` per source scope. See
  §2.6 and §7 checkpoint 8.10.

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

### 2.4 Readiness to delete the AST LLVM lowering (assessment, session 12)

**The shift is complete functionally; the deletion itself is not done.** The AST
LLVM lowering is unreachable for Chemical modules — `ASTProcessor::compile_module`
returns `mir_compile_module(gen, module)` unconditionally — and the last
hard-requirement gap (debug info) is now closed from MIR (session 12, §2.2). The
suites that exercise the LLVM backend are green (`--llvm` 2298/2298,
`--llvm --async` 53/53, `--llvm --libs` builds).

**DONE (2026-10-13, session 13).** The AST LLVM lowering has been deleted:

1. ~~The C-header legacy use~~ **removed.** `LabBuildCompiler` no longer calls
   `node->code_gen_declare` for `mod->headers`; C-header symbols are declared
   from MIR on demand (like every `@extern`).
2. ~~The mechanical deletion~~ **done.** `backend/LLVM.cpp`,
   `backend/LLVMCoroutine.cpp`, `Operation.cpp`, `backend/LLVMGen.{h,cpp}`, the
   `#ifdef COMPILER_BUILD` `llvm_*`/`code_gen_*` blocks across 167 `ast/` files,
   `Codegen`'s AST methods, `ASTCompiler.cpp`'s dead `compile_module` body and
   `code_gen_*` wrappers, and `DebugInfoBuilder`'s AST methods are all gone
   (**−15,661 / +91 lines**). `LLVMBackendContext`'s out-of-line definitions moved
   to a new `backend/LLVMBackendContext.cpp`.
3. **DI polish (not a blocker).** `llvm.dbg.declare`/`dbg.value` for locals and
   `DILexicalBlock`s are still missing; the §5.1 marker set
   (`DICompileUnit`/`DISubprogram`/`DILocation`/`DIBasicType`/`DICompositeType`)
   is present.

Verified after deletion: `--llvm` **2298/2298**, `--llvm --async` **53/53**,
`--tcc` **2300/2300**, `MIRTests` **1111 checks**, and `--mode debug_complete`
still emits DI. Remaining non-functional work is the evaluation's action plan
(consolidation/perf/size), not deletion.


### 2.5 Session 14 (2026-10-14) — the dead legacy shell is gone

Session 13 deleted the AST lowering, but `Codegen`, `DebugInfoBuilder` and the
backend context still carried its *entire* API as declarations with no
definitions, plus three orphan headers. Session 14 removes that shell so the LLVM
target is only the MIR → LLVM driver:

- **`compiler/Codegen.h` 855 → 257 lines.** Deleted the destructible queue
  (`Destructible`/`DestructibleKind`, `destruct_nodes`, `pinned_slots`,
  `pinned_drop_flags`), the dyn-object helpers (`fat_pointer_type`,
  `pack_fat_pointer`, `get_dyn_obj_impl`, `assign_dyn_obj*`), all destruct
  machinery (`destruct`, `enqueue_destructible`, `conditional_destruct`,
  `dispatch_destruct_jobs`, `loop_array_destructor`, `find_drop_flag`,
  `set_drop_flag_for_node`, `create_destructible_for`), the AST
  declaration/definition API (`declare_nodes`, `compile_nodes`, `external_*`,
  `create_function`, `declare_function`, `define_function`,
  `declare_weak_function`, `create_nested_function`,
  `cleanFunctionEntryBlock`), operator lowering (`operate`, `implicit_cast`,
  `implicit_cast_constant`), the loop/terminator helpers (`current_loop_exit`,
  `current_loop_continue`, `redirect_return`, `CheckNullCondBr`, `DefaultRet`,
  `FunctionRet`, `SetInsertPoint`, `CreateBr`, `CreateRet`, `CreateCondBr`,
  `CreateUnreachable`, `has_current_block_ended`), the per-task caches
  (`mod_ptr_cache`, `ctx_ptr_cache`, `evaluated_func_calls`, `implicit_args`,
  `trait_impl_func_map`), the coroutine state (`current_coro`,
  `LLVMCoroContext`) and the write-only leftovers (`is64Bit`, `current_module`,
  `interface_module`, `current_function`, `destroy_current_scope`, `inbounds`).
- **`compiler/Codegen.cpp` 1,229 → 886 lines.** Deleted the free helpers that
  served the same lowering (`createEntryBlock`, `to_linkage_type`, `create_func`,
  `createDropFlag`, `create_destructible`, `create_arr_destructible`,
  `create_destructible_with_drop_flag`, `get_destructible_type_node`,
  `create_destructible_for`, `create_return_of_type`, `has_allocated_storage`,
  `getDynInterfaceDef`, `needsMemcpy`,
  `LLVMArrayDestructor::~LLVMArrayDestructor`) and the uncalled drop-flag /
  safe-terminator wrappers. The constructor also loses the unused `is_64_bit`
  argument and `module_init` its uncalled-for `ModuleScope*`.
- **Deleted files:** `compiler/backend/LLVMCoroutine.h` (its `.cpp` went in
  session 13), `compiler/backend/include/LLVMArrayDestructor.h`,
  `compiler/utils/TraitImplFuncMapKey.h` (only consumer was
  `trait_impl_func_map`).
- **`DebugInfoBuilder` 152 → 95 lines (`.cpp` 240 → 82).** Kept only what the MIR
  emitter uses: `isEnabled`, `isOptimized`, `builder`, `diCompileUnit`,
  `createDiCompileUnit`, `update_builder`, `finalize`,
  `start/end_di_compile_unit`. The AST-shaped half
  (`create_struct_type`, `create_replaceable_type`,
  `create(FunctionTypeBody*, …)`, `declare`, every scope method, `instr`,
  `di_loc`) is gone: `instr`/`di_loc` had **no callers**, and `di_loc`'s
  `#ifdef DEBUG` precondition ("scope for a location cannot be a di compile
  unit") was unsatisfiable once only the compile unit is ever pushed.
  `CodegenMir` builds its own `DISubprogram`/`DIType`/`DILocation`.
- **`LLVMBackendContext`** no longer holds a `Codegen*`. MIR lowers
  `Drop`/`Destroy`/`MemCpy` and atomics as first-class ops, so `forget` is the
  same no-op the C backend has and the remaining hooks are inert; they stay only
  to complete the `BackendContext` vtable, which the *comptime interpreter* still
  calls for `intrinsics::forget`/`memcpy` and the atomic builtins.

**AST-side leftovers (same session).** With the AST lowering gone, `ast/**` was
re-audited for LLVM-specific code. It is almost entirely comment noise now (94
`llvm` mentions, ~85 of them prose): no `llvm_value`/`llvm_pointer`/`llvm_type`/
`llvm_destruct`/`code_gen*`/`add_member_index` declarations or definitions remain
anywhere, and no `ast/` header includes an LLVM header. Three real leftovers were
removed:

- `ExtractionKind::ReinterpretLLVMValue` — zero references; the only LLVM-specific
  member of an otherwise lambda-extraction enum (`create_extraction_value_type`
  already fell through its `default` arm for it).
- `FunctionParam::calculate_c_or_llvm_index` and
  `FunctionType::c_or_llvm_arg_start_index` — the deleted AST backend's sret/self
  argument offset, with **no callers**. MIR computes the live equivalent itself
  (`MIRLowerer::needs_sret_return`), so these two were a dead duplicate of the ABI.
- Stale comments in `ChainPart.h`, `GenericInstIdentifier.h` and `ValueNode.h` that
  still named `llvm_type` / `llvm_value` / `code_gen`.

Deliberately **kept**, because they are not legacy AST lowering:

- `intrinsics::llvm::atomic_*` (`LLVMNamespace` + the `InterpretLLVMAtomic*`
  compiler decls in `ast/utils/GlobalFunctions.cpp`). The dependency runs the other
  way round: this is the *user-facing intrinsic surface* that `lang/libs/atomic`
  calls on its LLVM branch, and `MIRLowerer::lower_atomic_intrinsic` recognizes it
  and lowers it to first-class MIR `Atomic*` ops — the front end no longer touches
  an IRBuilder. Renaming the namespace would be a language/API change, not cleanup.
- `prepare_target_data` / `init_target_data`: live target *data* initialisation
  (windows/linux/musl/is64Bit/…), called from `LabBuildContext.cpp`. The `Compiler`
  build uses LLVM's `Triple` parser under `#ifdef COMPILER_BUILD`; `TCCCompiler`
  has a hand-rolled string-splitting twin, so no LLVM header leaks into the
  LLVM-free compiler. (It does sit oddly in `ast/utils/GlobalFunctions.cpp`; moving
  it is a refactor for later.)

The emitter is confirmed backend-pure: **`compiler/CodegenMir.cpp` includes no
`ast/` header at all** (only `Codegen.h`, `compiler/mir/*` and LLVM), and nothing in
`compiler/mir/` includes an LLVM header.

**Behaviour restored while deleting.** The deleted `create_func` was the only
place applying the per-function unwind policy, so `-fno-unwind-tables` (and
`-fno-asynchronous-unwind-tables`) had silently become no-ops and a
`debug_complete` build no longer forced unwind tables for the `-bt` gdb
backtraces. `Codegen::mir_function` now applies the same policy when it creates a
function: `uwtable` in `debug`/`debug_complete` unless `fno_unwind_tables`,
otherwise `nounwind`. That is safe because MIR never emits
`invoke`/`landingpad` (the `Throw` terminator has no lowering — Chemical has no
exceptions). Verified on `lang/compiled/mirdel`: the `std` module has 477
`uwtable` in `debug_complete`, **0** with `-fno-unwind-tables` (479 `nounwind`),
and 0 `DICompileUnit` in the default mode.

**Verification (session 14, from the committed sources):**

| Command | Result |
|---------|--------|
| `cmake --build --target Compiler TCCCompiler MIRTests` | all build |
| `./cmake-build-debug/MIRTests` | 1111 checks OK |
| `./scripts/test.sh --llvm` | **2298 / 2298** |
| `./scripts/test.sh --tcc` | **2300 / 2300** |
| `./scripts/test.sh --llvm --async` | 53 / 53 |
| `./scripts/test.sh --tcc --async` | 53 / 53 |
| `./scripts/test.sh --llvm --libs` | 718/719 — the failure is the pre-existing, compiler-independent `test_fs_directory_iteration` |
| probe `lang/compiled/mirdel` (LLVM vs C) | identical output (`total=7`, `dtor 3`); `debug_complete` carries `.debug_info`/`.debug_line`/`.eh_frame` |

`test_fs_directory_iteration` (`lang/tests/libs/fs/`) fails **identically** in both
prebuilt lib-test executables (`lang/tests/build/tests.exe` and
`tests-tcc.exe`), run directly with no compiler involved: `fs::read_dir` finds
neither the two files nor the subdirectory the test creates (`is_dir` comes from a
per-entry `lstat` in `lang/libs/fs/posix/directory.ch`). Pre-existing `fs` /
environment issue, not a codegen regression — it is *not* fixed here.

**What remains in the migration is not deletion:** ~~the DI polish of §5.1~~
(done in session 15, §2.6) and the evaluation's action plan (compile time / peak
memory / binary size) in `lang/docs/mir-migration-evaluation.md`.

### 2.6 Session 15 (2026-10-14) — debug info is complete: locals and lexical blocks

Checkpoint **8.10** is done. The MIR→LLVM path already emitted
`DICompileUnit`/`DISubprogram`/`DILocation`/DI types (session 12); what was
missing was everything a debugger needs to *name a local* and to *scope it*.
Nothing in MIR recorded that a place had a source name, and the LLVM emitter had
no scope table, so `--mode debug_complete` produced line info with no variables.

**MIR now carries the metadata (debug builds only).**

- `MIRPlaceDef::debug_name` — the source identifier of a named place, as an
  interned String `ConstantId` (`MIR_INVALID_ID` = a temporary / call result /
  frame field). `MIRPlaceDef::debug_scope` — the lexical scope it was declared
  in.
- `MIRLexicalScope` (new) — `parent`, the half-open `inst_begin`/`inst_end`
  instruction range, and the scope's own source location.
- `MIRBuilder::set_place_debug_name` / `push_lexical_scope` / `pop_lexical_scope`.
  `MIRLowerer::bind_name` records the name + current scope; the lowerer wraps
  every statement body (`lower_scope`, the `Block`/`Scope`/`UnsafeBlock`
  statement cases, `lower_scope_value`) in a lexical scope. All of it is behind
  `MIRLowerer::set_debug_info`, wired from `options->debug_info` next to the
  existing source-location provider, so release / `debug_quick` records nothing
  and pays nothing. `lexical_scope_of_inst` picks the innermost scope of an
  instruction by the highest matching index (scopes are pushed LIFO, so ranges
  nest and a nested scope always has a higher index).

**The emitter consumes it.** `CodegenMir` creates a `DILexicalBlock` per
`MIRLexicalScope` (parents are created first — the table is in push order),
attaches each instruction's `DILocation` to the innermost block covering it, and
emits a `DILocalVariable` + `dbg.declare` record on every named place's alloca,
with the variable owned by the scope it was declared in. Locals stay in memory
in MIR (an `alloca` plus explicit load/store — no SSA-local promotion in either
backend), so `dbg.declare` is the semantically right intrinsic and no `dbg.value`
is needed; `AlwaysPreserve` keeps the variables through `DIBuilder::finalize`.

**Measured** on `lang/compiled/perfprobe` (`--mode debug_complete --assertions
--out-ll-all`, 4 modules):

| Check | Before | After |
|---|---:|---:|
| IR `DILexicalBlock` | 0 | **491** |
| IR `DILocalVariable` | 0 | **1084** |
| IR `dbg.declare` records (`#dbg_declare`) | 0 | **1084** |
| DWARF `DW_TAG_variable` | 0 | **1084** (named `self`, `value`, `index`, `i`, …) |
| DWARF `DW_TAG_lexical_block` | 0 | **61** |

(`DW_TAG_lexical_block` < the IR count because LLVM prunes scopes that own no
variable.) `DILocation` rises 2734 → 3016 as locations move from the subprogram
to their lexical block. Program output is unchanged.

**Gating verified:** the same module built `--mode debug_quick` emits **0**
`DI*` nodes, **0** `#dbg_*` records, and the executable has **no** `.debug_info`
section.

**No `dbg.value` by design.** The requirement in §5.1 lists
`llvm.dbg.declare`/`llvm.dbg.value`; `dbg.declare` covers memory-resident locals,
which is all MIR has. A note in the emitter records the reasoning so a later
optimizer-driven change can revisit it.

**Smoke check added:** `scripts/mir-debug-info-check.sh` builds its own fixture
(a struct method, an `if/else` with a nested local, a loop) and asserts:
`debug_complete` → `DICompileUnit`/`DISubprogram`/`DILocation`/`DILexicalBlock`/
`DIBasicType`/`DICompositeType`/`DILocalVariable`/`dbg.declare` in the IR **and**
`.debug_info`/`.debug_line`/`DW_TAG_variable`/`DW_TAG_lexical_block` in the
executable; `debug_quick` → none of them; both print identical output. It exits
non-zero on any gap, and it is *sensitive* — the pre-change compiler would have
failed it on `DILexicalBlock` and `dbg.declare`, both 0. (`--no-build` reuses the
existing binary.) It is a script the checkpoint runs, not a GitHub workflow step:
the repo's workflows test released binary assets from `chemicallang/chemical`,
not a source build, so there is no existing build job to attach it to.

**Suites (all green, exit status captured):** `MIRTests` **1111 checks**,
`--llvm` **2298/2298** (also re-run with `--mode debug_complete` — that is the run
that actually executes the new DI code — **2298/2298**), `--tcc` **2300/2300**,
`--tcc --async` **53/53**, `--llvm --async` **53/53**,
`./scripts/mir-debug-info-check.sh` **PASS**.

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

### 5.2b No backend may read the AST — every backend works from MIR

Binding rule, same text as `mir-implementation-plan.md` §0 rule 9 and
`mir-design.md` §16. The AST is a front-end artifact (parser, symres,
type-check, `MIRLowerer`); **no code generator may touch it**. `CodegenMir.cpp`
and `MIREmitter.cpp` are dispatchers over MIR only: types/layout from
`MIRTypeTable`/`MIRLayout`, calling convention from `MIRABI`, per-function facts
from `MIRFacts`, names/mangling/linkage and constants from the MIR module,
declarations from MIR declaration tables (`MIRDeclPrinter` today;
`MIRTypeDecl`/`MIRPrototype`/`MIRGlobal` as they land). No `ast/` include, no
`ASTNode*`/`BaseType*`/`StructDefinition*`/`FunctionDeclaration*` parameter, no
source-node walk, no re-derivation of layout, ABI, declarations, destructor
rules or lifetime from source form. Only the lowering boundary
(`MIRTypeBuilder`, `MIRLowerer`) and the front-end declaration collection pass
may see the AST; they *feed* MIR and are never a backend's source of truth. A
fact a backend needs that MIR does not publish is **added to MIR** — a field or
type fact on the record, a declaration record, a fact pass — never fetched from
the AST. This is what keeps C and LLVM from drifting and makes a future
JVM/JS/WASM target a thin dispatcher + printer rather than another front end.
Legacy `ast/*::llvm_type(Codegen&)` helpers predate this rule: do not call or
extend them from new code.

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
- **8.9 — Delete the AST LLVM lowering. ✅ DONE (session 13).** Removed
  `backend/LLVM.cpp`, `backend/LLVMCoroutine.cpp`, `Operation.cpp`,
  `backend/LLVMGen.{h,cpp}`, `Codegen`'s AST methods, `ASTCompiler.cpp`'s dead
  body/`code_gen_*`, `DebugInfoBuilder`'s AST methods, and the
  `#ifdef COMPILER_BUILD` codegen methods in `ast/**` (−15,661 / +91 lines).
  `LLVMBackendContext` moved to `backend/LLVMBackendContext.cpp`. Build green,
  `MIRTests` + TCC + LLVM suites green.
- **8.11 — Remove the dead legacy shell. ✅ DONE (session 14).** Deleted the
  dead `Codegen`/`DebugInfoBuilder` declarations and wrappers the AST lowering
  left behind, the three orphan headers, and the vestigial `destruct_nodes`-based
  `forget`; restored the per-function unwind policy (`uwtable`/`nounwind`) that
  the deleted `create_func` used to apply. See §2.5.
- **8.10 — Debug-info parity & sign-off. ✅ DONE (session 15).** MIR→LLVM
  emits `DICompileUnit`/`DISubprogram`/`DILocation`/DI types and a real line
  table (session 12), and now also `DILocalVariable` + `dbg.declare` records for
  every named local and a `DILexicalBlock` per source scope, mirrored into the
  executable's DWARF (`DW_TAG_variable`, `DW_TAG_lexical_block`). No
  `dbg.value` is emitted: MIR keeps every local in memory (an `alloca` plus
  explicit load/store), so `dbg.declare` is the correct intrinsic — the same
  thing the deleted AST backend's `DebugInfoBuilder::declare` did. The smoke
  check is `scripts/mir-debug-info-check.sh`, which asserts both the presence of
  DI in `debug_complete` **and its absence in `debug_quick`**. See §2.6.

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

Run the script — it builds its own fixture, checks both modes and exits non-zero
on any gap (see §7 checkpoint 8.10):

```bash
./scripts/mir-debug-info-check.sh              # build the Compiler, then check
./scripts/mir-debug-info-check.sh --no-build   # use the existing binary
```

It asserts that a `debug_complete` build carries `DICompileUnit`, `DISubprogram`,
`DILocation`, `DILexicalBlock`, `DIBasicType`, `DICompositeType`,
`DILocalVariable` and per-local `dbg.declare` records in the IR plus
`.debug_info`/`.debug_line`/`DW_TAG_variable`/`DW_TAG_lexical_block` in the
executable, that a `debug_quick` build carries **none** of them, and that both
builds print identical output.

Manual spot check:

```bash
grep -E "DICompileUnit|DISubprogram|DILocation|DILexicalBlock|DIBasicType|DICompositeType|dbg.declare" \
    lang/compiled/my_test/build/modules/*/llvm_ir.ll | head
readelf --debug-dump=info lang/compiled/my_test/main.exe | grep -E "DW_TAG_variable|DW_TAG_lexical_block" | head
readelf --debug-dump=decodedline lang/compiled/my_test/main.exe | head   # or llvm-dwarfdump
```

> LLVM ≥ 19 prints `dbg.declare` as a **debug record** (`#dbg_declare(ptr %1, !23,
> !DIExpression(), !24)`) rather than an `@llvm.dbg.declare` call. Both spellings
> are emitted by `DIBuilder::insertDeclare`; the script accepts either.

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
- LLVM (MIR → LLVM; the AST lowering is gone): `compiler/Codegen.{h,cpp}` (thin
  MIR → LLVM state, target machine and object/link driver),
  `compiler/CodegenMir.cpp` (the emitter — `mir_type`, `mir_fn_type`,
  `mir_function`, `mir_emit_function`, `mir_di_type`, `mir_declare_globals`),
  `compiler/backend/DebugInfoBuilder.{h,cpp}` (DI compile unit),
  `compiler/backend/LLVMBackendContext.{h,cpp}`, and `compiler/ASTCompiler.cpp`
  (`mir_compile_module` plus `mir_emit_llvm_module`).
- Deleted in 8.9 / 8.11: `compiler/backend/LLVM.cpp`,
  `compiler/backend/LLVMCoroutine.{h,cpp}`, `compiler/backend/LLVMGen.{h,cpp}`,
  `compiler/Operation.cpp`, `compiler/backend/include/LLVMArrayDestructor.h`,
  `compiler/utils/TraitImplFuncMapKey.h`, and the `llvm_*`/`code_gen_*` methods in
  `ast/**`.
- Keep: `compiler/backend/CLANG.cpp`, `clang_*`, `llvm-ar.cpp`, `chem_clang.cpp`,
  `compiler/ctranslator/CTranslator.{h,cpp}`.
- Driver: `compiler/lab/LabBuildCompiler.cpp` (LLVM job ~lines 2080–2180),
  `compiler/ASTProcessor.cpp` (MIR wiring ~lines 1676–2160).
- Tests: `lang/tests/common/`, `lang/tests/src/`, `lang/tests/async/`,
  `lang/tests/negative/`, `compiler/mir/tests/mir_tests.cpp`.
