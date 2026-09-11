# Pure-Chemical Atomic Library — Compiler Feature & Implementation Plan

## Amended Objectives (2026-09-09)

**What went wrong with the preamble approach:**
The original §13.5 strategy was implemented as ~560 lines of raw C text written into
`prepare_translate()` in `2cASTVisitor.cpp`. This is the WRONG approach because:

1. **It's a downgrade, not an upgrade** — if we wanted to write C, we should write a `.c`
   file and include it (which is what `cruntime/` did). Writing raw C strings in C++ code
   is harder to maintain, harder to test, and doesn't prove our language is capable.

2. **The C backend's atomic methods already work** — `ToCBackendContext::atomic_load()`,
   `atomic_store()`, `atomic_cmp_exch_*()`, `atomic_op()` already emit the correct
   `__atomic_*_N` suffixed C calls as `RawLiteral` values. The preamble was only needed
   to provide `static inline` definitions for these symbols, bypassing the BackendContext.

3. **If something can be written in C, it should be writable in Chemical** — the goal is
   to introduce missing language features (like inline assembly) so that the atomic library
   can implement CAS, fences, and barriers **in `.ch` files**, not in C++ compiler code.

**Corrected approach:**

- **Phase 1**: Revert the preamble. The C backend's atomic methods already emit correct
  `__atomic_*_N` calls. On posix, link `-latomic` to resolve them. On Windows (no
  libatomic), the C backend methods need to emit self-contained C code using:
  - `volatile` accesses for load/store
  - `__asm__ __volatile__("" ::: "memory")` for compiler barriers
  - Platform-specific inline asm for CAS (lock cmpxchg on x86, ldxr/stxr on ARM64)
  - Statement-expression CAS loops for RMW operations

- **Phase 2**: Make the C backend methods emit self-contained code instead of calling
  external symbols. Each `atomic_*` method builds a C expression string that uses only
  volatile, compiler barrier, and inline asm — no `@extern`, no `-latomic`.

- **Phase 3**: Delete `cruntime/` and `preamble_posix.ch`. The atomic library becomes
  pure Chemical, using `intrinsics::llvm::atomic_*` calls that the C backend translates
  to self-contained C code.

- **Phase 4** (future): Add inline assembly as a Chemical language feature so the atomic
  library can implement CAS directly in `.ch` files instead of relying on the C backend
  to generate the right C code. This is the real "upgrade" — making Chemical expressive
  enough to write everything that C can write.

Status: implementation plan — amended for Windows (see **§14 Windows amendments**; the
amendments are authoritative where they conflict with earlier text). In particular,
**§13.5 (generic calls + pure-C preamble definitions) is the primary emission strategy on
all platforms**, which makes every Windows target zero-external-dependency: Windows has no
usable libatomic (see §14.1), so the §13.4 note "posix ⇒ link `-latomic`" applies to posix
targets only and only as a fallback to §13.5.
Scope: make `lang/libs/atomic` shippable with **no C files and no C headers**, by extending
the Chemical compiler (primarily the C/2c backend) so that generated C is *analogous* to the
C runtime the library ships today.
Constraints baked in:

- Generated C must compile with **any C compiler**: GCC, Clang, TinyCC.
- Must support **any platform**: Windows, Linux, macOS (and be extensible to FreeBSD/Android).
- The **target is known beforehand**, so the compiler emits target-specific C — no `#ifdef`
  ladders, no C preprocessor conditionals for platform selection. Chemical's `def.*` comptime
  system (`def.windows`, `def.tcc`, `def.x86_64`, `def.aarch64`, …) does all filtering at
  compile time, before a single byte of C is emitted.
- The LLVM backend already has full atomic support (LLVM IR models atomics natively), so this
  plan is almost entirely about the **C backend (2c)** plus a small set of language-level
  features the fallback implementations need.

---

## 0. Summary

| # | Feature | Why it's needed | Priority |
|---|---------|-----------------|----------|
| F1 | Implement the 6 atomic intrinsics in `ToCBackendContext` | Today they are silent no-ops; this is **the** blocker | P0 |
| F2 | Target-aware emission + `libatomic` linking rules | TCC cannot use the generic builtin forms everywhere; Windows has no libatomic | P0 |
| F3 | Pointer-width atomics (`AtomicUsize` / pointer cells) | The current C runtime provides pointer-sized ops via `_Generic`/`UINTPTR_MAX` selection | P1 |
| F4 | `volatile` qualifier | Every synthesized fallback (CAS loops, spin paths) is incorrect at `-O2` without it | P1 |
| F5 | Compiler-only barrier (`atomic_signal_fence`) intrinsic | The universal `__asm__ __volatile__("" ::: "memory")` compiler barrier used by all fallbacks | P1 |
| F6 | Backend capability queries (`intrinsics::supports` extension) | The library must pick builtin path vs. fallback path **at comptime**, per target | P1 |
| F7 | Pure-Chemical fallback layer (CAS synthesis, word-tearing emulation, fence synthesis) | The Windows(+TCC) corner where neither builtins nor libatomic cover everything | P1 |
| F8 | Interpretation-mode backend context | `--arg-interpret` currently has no `backend_context` at all — atomic comptime calls crash/no-op | P5 |
| F9 | Intrinsic declaration fixes (return types, param indices, typos) | Existing bugs in the comptime intrinsic declarations | P0 (trivial) |

Phases: **P0** probes & baseline tests → **P1** F1+F9 (C backend core) → **P2** F2 + library
rewrite (delete `cruntime/`) → **P3** F4+F5+F6 → **P4** F7 (Windows corner) → **P5** F3+F8 +
polish. Full DAG in §7.

---

## 1. Background

### 1.1 What the atomic library is today

```
lang/libs/atomic/
├── build.lab                      # C backend: adds cruntime/atomic.c as a C-file module
├── src/
│   ├── atomic.ch                  # public API — comptime funcs that branch on backend
│   └── types/
│       ├── atomic_u8.ch           # pure-Chemical wrappers (already fine)
│       ├── atomic_u16.ch
│       ├── atomic_u32.ch
│       └── atomic_u64.ch
└── cruntime/                      # ← the thing we want to delete (~2500 lines of C)
    ├── atomic.c                   # dispatches to win/ or nix/ headers
    ├── nix/
    │   ├── atomic.h               # TCC → direct libatomic symbols; GCC/Clang → C11 _Atomic casts
    │   └── atomic_cpp.h           # C++ <atomic> shim (bionic-derived)
    └── win/
        ├── atomic.h               # FFmpeg-derived Win32 stdatomic compat (Interlocked*)
        └── atomic_compat.h        # extra acquire/release emulation helpers
```

The public API (`src/atomic.ch`) is a set of **comptime functions** that branch on the backend
at compile time:

```chemical
public comptime func atomic_load_u64(x : %runtime<*u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(intrinsics::get_backend_name() == "C") {
        return %runtime_value(atomic_load_u64_explicit(x, order as int)) as u64   // @extern → cruntime
    } else {
        return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope_to_int(llvm_atomic_sync_scope.system)) as u64
    }
}
```

- **LLVM path** works today: `intrinsics::llvm::atomic_*` are compiler-declared functions
  interpreted at comptime; each call reaches `BackendContext` and produces real LLVM IR.
- **C path** today compiles 44 `@extern` declarations against `cruntime/atomic.c`, which
  itself includes one of four large C headers depending on the OS and C compiler — selected
  by **C preprocessor macros** (`#ifdef _WIN32`, `#ifdef __TINYC__`, `#ifdef __aarch64__`, …).
  This is exactly the `#ifdef` machinery we want gone.

### 1.2 The comptime backend-context contract (how a comptime call becomes machine code)

`compiler/lab/BackendContext.h:21` defines the escape hatch by which comptime code talks to
the active backend. Its own doc comment states the design intent:

> *"example: this can be used to generate code, based on user's compile time function calls,
> for example user calls memcpy using std::mem::cpy <-- C backend will write normal memcpy …
> and LLVM backend will write a normal llvm memcpy intrinsic function call"*

The contract, per backend job:

| Stage | What happens |
|-------|--------------|
| Job setup | `LabBuildCompiler.cpp:1758-1759` installs `ToCBackendContext` for TCC/C jobs; `:2021-2023` installs `LLVMBackendContext` for LLVM jobs; `:3303-3306` installs the C context for build.lab JIT. |
| Comptime call | `intrinsics::llvm::atomic_load(...)` etc. (`ast/utils/GlobalFunctions.cpp:2427-2700`) validate orders/scopes, then call `backend_context->atomic_load(ptrVal, …)` — **during codegen of the enclosing function**, with the argument `Value*`s in codegen position. |
| LLVM backend | `compiler/backend/LLVM.cpp:2896-2974` emits real IR: `CreateFence`, `CreateLoad/Store` + `setAtomic`, `CreateAtomicCmpXchg` (extracts `{old, success}`), `CreateAtomicRMW`, wrapping results via `pack_llvm_val` → `ExtractionValue(ReinterpretLLVMValue)`. |
| C backend | `preprocess/2c/2cBackendContext.h:47-82`: **every one of the six methods is a stub** — emits nothing and returns the wrong thing (`atomic_load` returns the *pointer*, `atomic_op` returns the *value* argument). Any use of the atomic library on the C backend today is silently wrong. |

Two existing precedents prove the C-side mechanism is viable:

1. **`mem_copy`** (`2cASTVisitor.cpp:7947`): receives `Value*` args and renders them with
   `visitor->visit(lhs)` — i.e. the C visitor *can* render arbitrary comptime-evaluated
   values into the output at the current position.
2. **`RawLiteral`** (`ast/values/RawLiteral.h`, rendered at `2cASTVisitor.cpp:7152`): a
   `Value` that carries a string and is written **verbatim** into the C output — perfect for
   returning a C expression from a comptime intrinsic, in *expression position*.

Additionally, the 2c backend already emits GNU **statement expressions** (`(*({ … }))`) for
sret struct returns, and TinyCC supports them — so multi-step C sequences (CAS loops) can be
emitted as expressions.

The C output is deliberately **self-contained**: `prepare_translate()`
(`2cASTVisitor.cpp:3219`) emits its own `bool`/`NULL`/`offsetof` definitions and fixed-width
`intN_t` typedefs chosen from the target (`is64Bit`/`win64`), because stdint.h/stddef.h are
not available in every environment the output must compile in. Atomic emission must follow
the same discipline (no new `#include`s).

### 1.3 The C runtime we are replacing — inventory

The headers encode, per compiler × OS × arch, the following behaviors that pure-Chemical +
compiler support must reproduce:

| Concern | nix/atomic.h | win/atomic.h |
|---|---|---|
| GCC/Clang | C11 `stdatomic.h`, `_Atomic` casts over plain memory | `__atomic_compare_exchange` builtins for 64-bit CAS; compiler-specific fences |
| TCC | **Direct declarations of libatomic symbols** `__atomic_load_1/2/4/8`, `__atomic_store_N`, `__atomic_compare_exchange_N`, `__atomic_exchange_N`, `__atomic_fetch_{add,sub,and,or,xor}_N` — comment: *"tcc is not capable to use generic C functions"* | `__atomic_compare_exchange` builtin (works on x86); manual CAS loops for everything else |
| MSVC | n/a | `Interlocked*` family, `_ReadWriteBarrier`, `MemoryBarrier` |
| Fences | `__atomic_thread_fence` / `atomic_thread_fence` | x86: `mfence` inline asm or compiler barrier; ARM: SDK `MemoryBarrier()` |
| `cpu_relax()` | per-arch: x86 `pause`, ARM `yield`, RISC-V `fence rw,rw` + dummy, PPC `or 1,1,1`, MIPS `ssnop`, fallback `nop`s | `_mm_pause` / no-op on ARM |
| 8/16-bit ops on Windows | n/a | **word-tearing emulation**: x86 inline asm (`xchgb`, `lock cmpxchgb`, `lock xaddw`) or, on ARM, aligned-32-bit-word read–mask–CAS loops (`ManualInterlockedExchange16/8`, `ManualInterlockedCompareExchange16/8`) |
| 64-bit ops on Windows/ARM64+TCC | n/a | CAS loops over `ManualInterlockedCompareExchange64` |
| Bitwise RMW on Windows/ARM64+TCC | n/a | CAS loops (`ManualInterlockedOr/Xor/And{,16,32,64}`) |
| aarch64 libatomic helpers | ~20 `extern inline` acq_rel/relax wrappers (`__aarch64_casN_*`, `__aarch64_ldaddN_*`, …) | n/a |
| Pointer atomics | `_Generic`-dispatched or `UINTPTR_MAX`-selected 4/8-byte variants | pointer-width `Interlocked*` on `_WIN64` |

**Latent bugs found in the header we are deleting** (evidence the rewrite is overdue):

- `nix/atomic.h` declares `__atomic_compare_exchange_8(x, expected, y, mo, mo2)` with **5
  arguments**, but the libatomic ABI is `(ptr, expected, desired, weak, success, failure)` —
  6 arguments. Under SysV x86-64 the `failure` order lands in `r9` uninitialized. Suspected
  silent UB; verify during Phase 0 probes, then enjoy deleting it.
- `nix/atomic.h` maps `atomic_fetch_sub_explicit` to `__atomic_sub_fetch_4` (the no-return
  "store new value" family) instead of `__atomic_fetch_sub_4` — wrong semantics.
- `src/atomic.ch` declares `atomic_store_u16_explicit(x : *mut void, …)` — should be
  `*mut u16` (typo, inconsistent with all siblings).
- `ast/utils/GlobalFunctions.cpp:2608-2611`: `InterpretLLVMAtomicCmpExchStrong` declares
  `expParam` with parameter index `0` (duplicate of `ptrParam`) — should be `1`.
- `ast/utils/GlobalFunctions.cpp:~2631`: `InterpretLLVMAtomicOperation` is declared returning
  `void`, yet `atomic.ch` casts its result (`… as u64`). The LLVM path happens to work because
  the returned `Value` is used regardless; the declared type is still wrong.

### 1.4 The rules the generated C must satisfy

1. **No headers added** — output stays self-contained like the existing preamble
   (`prepare_translate()`), because tcc's include package / Alpine / musl environments vary.
2. **No target `#ifdef`s** — the compiler knows `windows/posix`, `tcc/clang`, arch, endianness
   and bitness from `comptime_scope.target_data` at emission time. Target selection happens
   in the compiler (C++) or in comptime Chemical (`def.*`), never in the emitted C.
   (The pre-existing `#ifdef __cplusplus` guards in the preamble are infrastructure, not
   target logic, and stay.)
3. **No libatomic on Windows** — there is none; whatever C we emit for Windows must be
   self-sufficient (builtins that compile inline, or pure-C fallback code we generate).
4. **One C file per target, for ALL C compilers — the user never tells us their C compiler.**
   The user selects only the *target* (e.g. `--target x86_64-windows-gnu`); there is no
   "which C compiler" input to codegen and there must never be one. For a given target, the
   emitted C is a single fixed artifact that compiles with gcc, clang and tcc alike.
   Concretely: the emitted C is restricted to the **intersection** of all three compilers'
   accepted constructs for that target — GNU `__atomic` builtins, the empty-asm statement,
   GNU statement expressions, `volatile` (see §12 C checklist). Where a compiler is missing
   a capability for a target, we emit the weakest common form (size-suffixed libatomic
   symbols on posix — all three compile and link those) or the pure-C fallback layer (F7)
   where no common form exists (Windows-ARM64+tcc). Link flags (e.g. `-latomic`) are chosen
   by *our driver*, which knows the target and its own backend — they never change the
   emitted C content. (Windows: since there is no libatomic, the common form on every
   Windows target is the §13.5 defs form or F7 fallback — see §14.1–§14.2.)
5. Plain-memory atomics only — Chemical variables are ordinary C objects; we deliberately do
   **not** use C11 `_Atomic` types (TinyCC doesn't support them — that's why `nix/atomic.h`
   bypasses `stdatomic.h` for TCC).

---

## 2. Mechanism chain: how a Chemical atomic call becomes C (end state)

The complete trace after this plan is implemented, for:

```chemical
var counter = AtomicU64.new(0)
counter.fetch_add(1, memory_order.acq_rel)
```

1. **Parse/symres/typecheck** — `fetch_add` resolves to `AtomicU64::fetch_add`
   (`src/types/atomic_u64.ch`), a `comptime func` taking `%maybe_runtime<u64>` and a
   `memory_order` default.
2. **Comptime interpretation of the body** — the interpreter runs
   `atomic_fetch_add_u64(&raw mut value, v, order)`. The `&raw mut value` argument is
   evaluated to a `Value*` that codegen can render (same class of value the `mem_copy`
   intrinsic renders today). The `memory_order.acq_rel` argument is a comptime enum value;
   `llvm_mem_order(order)` maps it to the LLVM-numbered `6` (`acquire_release`).
3. **Intrinsic dispatch** — `intrinsics::llvm::atomic_op(...)` →
   `InterpretLLVMAtomicOperation::call` (`GlobalFunctions.cpp:~2650`) → validation →
   `backend_context->atomic_op(BackendAtomicOp::Add, ptr, value, AcquireRelease, System)`.
4. **C backend emission** (new, F1) — `ToCBackendContext::atomic_op`:
   - maps `AcquireRelease → memory_order_acq_rel (4)` (C11 numbering),
   - renders `ptr` and `value` to C expression strings via a scratch writer
     (e.g. `(&counter.value)` and `1`),
   - since `Add` maps to a direct builtin, returns
     `RawLiteral("__atomic_fetch_add((&counter.value), 1, 4)", type=u64)`.
5. **Rendering** — when the C visitor lowers the enclosing expression, it reaches the
   `RawLiteral` (in the exact AST position where the comptime call was) and writes the string
   verbatim (`2cASTVisitor.cpp:7152`). The enclosing statement becomes:

   ```c
   __atomic_fetch_add((&counter.value), 1, 4);
   ```

6. **Link** — on `linux-x86_64-tcc` the job links `libatomic` (F2); on clang/gcc the builtin
   compiles inline; on `windows-x86_64-tcc` the same builtin compiles inline in TCC (Phase 0
   probe confirms) or falls back to the pure-Chemical fallback layer (F7).

Nothing in this chain requires `#ifdef`, C macros, or precompiled C objects.

---

## 3. Feature specifications

### F1 — Atomic intrinsics in the C backend (`ToCBackendContext`)  [P0, the blocker]

**Problem.** All six virtuals in `preprocess/2c/2cBackendContext.h:47-82` are stubs.

**Design.**

#### F1.1 Memory-order and scope mapping

`BackendAtomicMemoryOrder` uses LLVM numbering (`compiler/lab/backend/atomics.h`); C11 wants
0–5. Map in C++ at emission time and emit integer literals:

| `BackendAtomicMemoryOrder` | C11 emission | Notes |
|---|---|---|
| `NotAtomic (0)` / `Unordered (1)` | **compile error** | invalid for these ops (mirrors LLVM backend behavior) |
| `Monotonic (2)` | `0` (relaxed) | for load/store/RMW |
| `Acquire (4)` | `2` | |
| `Release (5)` | `3` | |
| `AcquireRelease (6)` | `4` | |
| `SequentiallyConsistent (7)` | `5` | |

Fence special case: LLVM's `atomic_fence` errors on monotonic and upgrades to `acq_rel`
(`LLVM.cpp:2896-2907`). The C backend must mirror this (upgrade monotonic → acq_rel) so both
backends agree on `atomic_fence(relaxed)`.

Sync scope: C11 has no single-thread scope. Decision: **validate but accept only `System`**
with a clean error for `SingleThread` (the library only ever passes `system`; failing loudly
beats silently diverging semantics). If single-thread scope is ever needed, it becomes a
follow-up that lowers to relaxed accesses + signal fences.

`memory_order.consume` is already mapped to `acquire` at comptime
(`convert_mem_order_to_llvm` in `atomic.ch`); document that Chemical's consume == acquire, as
LLVM does.

#### F1.2 Rendering argument values to C strings (scratch writer)

Add to `ToCAstVisitor`:

```cpp
// Renders an arbitrary value into a standalone C expression string.
std::string render_value(Value* value);
```

Implementation options (pick during implementation):
- a scratch `BufferedWriter` swapped into the visitor while visiting the value, or
- a nested `ToCAstVisitor` sharing the context (`comptime_scope`, mangler, naming state) but
  owning a throwaway buffer.

Constraints to enforce with clear errors:
- Argument values must be renderable **in expression position** (lvalues, loads, casts).
  Pointer arguments to atomic ops are lvalue-ish addresses (`&raw` of fields/variables) —
  the `mem_copy` precedent shows this works.
- Values that would require statement emission (sret temporaries, constructor calls) are
  rejected with a diagnostic ("atomic argument must be a simple lvalue/expression").

#### F1.3 Pointee typing

The pointer argument's type (`ptr->getType()` → `PointerType` → pointee) determines:
- the C type name used in synthesized statement expressions (rendered via the existing type
  visitor with the `array_types_as_subscript`-style care the zero-init path uses), and
- the result `Value`'s Chemical type (pointee type for load/RMW; `bool` for CAS; `void` for
  store/fence).

Only **integer pointees** (`u8/u16/u32/u64` and signed equivalents) are accepted in F1;
pointer pointees come with F3. Non-integer, non-struct-sized types error out.

#### F1.4 Per-intrinsic emission

> ⚠️ **Superseded by §13.4 (P0 probe results)**: the `_n` generic spellings shown in this
> subsection's examples do NOT work under TCC (unresolved references — §13.1). Emit the
> **size-suffixed `__atomic_*_N` forms** (with exact per-size preamble declarations) or the
> §13.5 generic+defs form; synthesize fences via dummy-cell exchange (no
> `__atomic_thread_fence` under tcc). The statement-expression synthesis pattern below
> remains valid — build it on suffixed ops.
>
> ⚠️ **Windows amendment (§14.2)**: prefer the **§13.5 form** — generic `_n` call sites +
> pure-C preamble definitions — on **all** platforms. The synthesized ops (RMW synthesis,
> fence synthesis) must then be built on the preamble **definitions**, not on suffixed
> libatomic symbols, so the artifact stays self-contained on Windows (no libatomic there,
> see §14.1).

Let `P` = rendered pointer expression, `V` = rendered value expression, `E` = rendered
expected-pointer expression, `mo1/mo2` = mapped C11 order literals.

**`atomic_load(ptr, order, scope) → pointee`**

```c
__atomic_load_n(P, mo1)
```

**`atomic_store(ptr, value, order, scope) → void`**

```c
__atomic_store_n(P, V, mo1)
```

**`atomic_fence(order, scope)`**

```c
__atomic_thread_fence(mo1)
```

**`atomic_cmp_exch_weak` / `atomic_cmp_exch_strong(ptr, expected, value, mo1, mo2, scope) → bool`**

```c
__atomic_compare_exchange_n(P, E, V, weak /*0 strong, 1 weak*/, mo1, mo2)
```

The C11 builtin natively performs the LLVM-mandated side effect (write the old value back
through `E`) and returns the success flag — the `{old, success}` `ExtractValue` dance the
LLVM backend does (`LLVM.cpp:2938-2958`) is unnecessary here. Return a `bool`-typed value.

**`atomic_op(op, ptr, value, order, scope) → pointee`** — mapping matrix:

| `BackendAtomicOp` | LLVM lowering | C emission | Notes |
|---|---|---|---|
| `Xchg` | `atomicrmw xchg` | `__atomic_exchange_n(P, V, mo1)` | returns old |
| `Add` | `atomicrmw add` | `__atomic_fetch_add(P, V, mo1)` | |
| `Sub` | `atomicrmw sub` | `__atomic_fetch_sub(P, V, mo1)` | |
| `And` | `atomicrmw and` | `__atomic_fetch_and(P, V, mo1)` | |
| `Or` | `atomicrmw or` | `__atomic_fetch_or(P, V, mo1)` | |
| `Xor` | `atomicrmw xor` | `__atomic_fetch_xor(P, V, mo1)` | |
| `Nand` | `atomicrmw nand` | `__atomic_fetch_nand(P, V, mo1)` | GNU extension; probe TCC, else synthesize (below) |
| `Max`/`Min`/`UMax`/`UMin` | `atomicrmw max…` | **synthesize** (below) | no builtin |
| `FAdd`/`FSub`/`FMax`/`FMin` | `atomicrmw f…` | **synthesize** | not exposed by the library; document as unsupported on C backend until needed |
| `UIncWrap`/`UDecWrap` | `atomicrmw …wrap` | **synthesize** | not exposed by the library |

**Synthesis pattern** (statement expression; `T` = pointee C type; used for anything without
a builtin). RMW that returns the old value, with success order `mo1`:

```c
(*({
    T __chx__old = __atomic_load_n(P, __ATOMIC_RELAXED__);
    for(;;) {
        if (__atomic_compare_exchange_n(P, &__chx__old, (T)(EXPR(__chx__old, V)), 1, mo1, mo1_failure)) break;
    }
    __chx__old;
}))
```

where `EXPR` is the combining expression (`__chx__old | V`, `(T)(__chx__old + V)`,
`__chx__old > V ? __chx__old : V` for signed max, …) and `mo1_failure` follows the C11 rule
(failure ≤ success, never release/acq_rel):

| success `mo1` | failure order used |
|---|---|
| relaxed (0) | relaxed (0) |
| acquire (2) | acquire (2) |
| release (3) | acquire (2) — release-only RMW still needs an acquire failure to retry correctly |
| acq_rel (4) | acquire (2) |
| seq_cst (5) | seq_cst (5) |

Names of scratch locals use the existing `__chx__` convention; uniqueness is per statement
expression (compound statement scopes make reuse safe).

Design note — alternative considered: emitting `static inline` helper functions into the
preamble (a "helper registry"). Rejected for now: per-call statement expressions need no
writer-level global state and TCC demonstrably handles the existing `(*({…}))` pattern;
revisit if nested statement expressions inside struct-return contexts ever conflict.

#### F1.5 One emitted form per target — the all-compilers intersection

> ⚠️ **Superseded by §13.3–§13.5 (P0 probe results)**: TCC supports neither the generic `_n`
> builtins nor a usable multi-operand family — the universal spelling is the **suffixed
> `__atomic_*_N` family** (or §13.5 generic+defs). The matrix below is retained for the
> *linking* column reasoning, but its "emitted C form" column is superseded. The
> best-current recommendation is §13.5: one artifact, no libatomic on any platform.
>
> ⚠️ **Windows amendment (§14.1)**: promoted from "recommendation" to **primary strategy** —
> Windows has no libatomic to link (MSYS2 ships `libatomic-1.dll` only inside
> `mingw-w64-x86_64-gcc-libs`, a runtime DLL; no static import-lib story). One artifact,
> zero external dependencies, all platforms. §14.2 defines the exact amended emission
> recipe; §14.7 lists the verification still owed before this is fully trusted.

`ToCBackendContext` has access to `comptime_scope.target_data`.

**Hard rule (restated from §1.4.4): the user selects only the target; codegen never sees a
compiler choice.** For each target configuration the backend emits exactly ONE form of each
atomic construct — the most capable form that compiles with **all three** compilers (gcc,
clang, tcc) on that target. The Phase 0 probe (P0.1) fills in the per-target cells; the
current best-estimate matrix:

| Target config | Emitted C form (single, for all compilers) | Link flags (our driver's decision) |
|---|---|---|
| linux/bsd, 64-bit arch | generic `__atomic_*_n` builtins | `-latomic` iff `def.tcc` (tcc emits calls to libatomic; gcc/clang inline the same C) |
| linux/bsd, 32-bit arch (i386, arm) | generic `__atomic_*_n` builtins | `-latomic` always (gcc/clang call out for 8-byte ops; tcc calls out for all) |
| macos, x86_64/aarch64 | generic `__atomic_*_n` builtins | **never** `-latomic` (macOS ships no libatomic); tcc-on-macos per probe → F7 fallback if builtins missing |
| windows x86/x64 (any C compiler) | **§13.5 defs form** (generic `_n` call sites + pure-C preamble defs — Windows amendment §14.2); F7 fallback behind it | **none** (never — no libatomic on Windows) |
| windows arm64 (any C compiler) | **F7 fallback layer** behind the same §13.5 preamble defs (no `__atomic_*` reliance at all) | **none** (never) |

Notes:
- The intersection rule means we never mix spellings per compiler — one target, one C
  artifact. Compiler-specific spellings (`_mm_pause`, `__sync_*`, `Interlocked*`, C11
  `_Atomic`) are **never** emitted.
- **Windows note (§14.1)**: the linux-host probe conclusion "gcc inlines the suffixed
  family, so `-latomic` is harmless-if-present" does **not** extend to Windows — there is
  nothing to link there. All Windows rows above therefore use the §13.5 defs form with
  **zero** link flags.
- The two universally-constructible floors on every target with all compilers: the
  **empty-asm statement** (`__asm__ __volatile__("" ::: "memory")` — F5) and **pure-C
  fallback code** (F7). Every target's form is either a builtin or something built on these.
- Contradictory TCC evidence to resolve in P0.1: `nix/atomic.h` *avoids* generic builtins
  under tcc ("tcc is not capable to use generic C functions"), while `win/atomic.h`
  *uses* `__atomic_compare_exchange` under tcc. The probe must determine exactly which
  builtin forms the bundled libtcc accepts, per target.

Size-suffixed symbol mapping (posix fallback only, §13.4): `__atomic_load_{1,2,4,8}(P, mo)`,
`__atomic_store_{1,2,4,8}(P, V, mo)`, `__atomic_exchange_{N}` → size by `sizeof(pointee)`,
`__atomic_compare_exchange_{N}(P, E, V, weak, mo1, mo2)` (note: **six** args — see §1.3 bug
list), `__atomic_fetch_{add,sub,and,or,xor}_{N}(P, V, mo)` — **never relied upon on Windows**
(§14.1). Under the primary §13.5 defs form, none of these appear in emitted C at all (the
preamble defs are the only place they exist).

#### F1.6 Intrinsic declaration fixes (F9 details)

- `InterpretLLVMAtomicOperation`: return type `void` → `any` (or the pointee via `getAnyType`),
  so the declared signature matches actual use (`atomic.ch` casts the result).
- `InterpretLLVMAtomicCmpExchStrong`: fix `expParam` index `0` → `1`
  (`GlobalFunctions.cpp:2608-2611`).
- Optional cleanup: rename/expose the `intrinsics::llvm::atomic_*` family as
  `intrinsics::atomic_*` (they are `BackendContext` calls, not LLVM-specific); keep the old
  path as an alias for compatibility during migration.

**Files touched:** `preprocess/2c/2cBackendContext.h`, `preprocess/2c/2cASTVisitor.{h,cpp}`
(`render_value`, possibly preamble tweaks), `ast/utils/GlobalFunctions.cpp`.

**Acceptance criteria.**
- A `temp.ch`-style isolated program (`lang/compiled/temp.ch` workflow) using all four
  `AtomicU{8,16,32,64}` types compiles under `TCCCompiler --emit-c`; the emitted C contains
  `__atomic_*` calls with correct C11 order literals.
- The emitted C compiles and runs identically under gcc `-O2`, clang `-O2` and tcc on the
  host (linux x86_64).
- LLVM backend output unchanged (existing tests keep passing).

---

### F2 — Target-aware emission + `libatomic` linking rules  [P0/P1]

**Problem.** Linking is part of correctness: TCC-generated calls to `__atomic_*_N` need
`-latomic` on posix; Windows must never get that link flag (no libatomic); i386 gcc/clang
also call out for 64-bit ops.

**Design.**

- The build script (`lang/libs/atomic/build.lab`) drops `ctx.c_file_module(...)` entirely and
  gains conditional system-lib linking (build.lab already runs with `def.*` available —
  precedent: `lang/libs/tls/build.lab:31` uses `comptime if(def.windows)`):

  ```chemical
  if(user_job.getTarget().c) {
      // The link decision is OUR DRIVER's job (def.tcc = C backend driving the job).
      // It never changes the emitted C — the same C inlines under gcc/clang and calls
      // out to libatomic under tcc.
      comptime if(def.posix && (def.tcc || def.i386)) {
          ctx.link_system_lib(user_job, "atomic", module)
      }
  }
  ```

  (`ctx.link_system_lib(job, name, mod)` exists — `BuildContextCBI.cpp:51`; the `chemical.mod`
  equivalent `link "atomic" if <cond>` is parsed at `parser/statements/LexStatement.cpp:630-664`
  and converted at `compiler/lab/mod_conv/ModToLabConverter.cpp:305-332`, so both module
  declaration styles support conditional linking.)

  > **Windows amendment (§14.3)**: with §13.5 promoted to primary strategy, the posix
  > `-latomic` branch above is a **posix-only fallback** to the self-contained defs form.
  > The Windows branch of F2 is `comptime if(def.windows) { /* link nothing */ }` — any
  > `libatomic` reference on a Windows target is a bug.

- The C backend's spelling choice (F1.5) is derived from the same `target_data` fields
  (`tcc`, `windows`, arch) — one source of truth, no `#ifdef`.

**Acceptance criteria.**
- `linux-x86_64-tcc` binary links and runs (libatomic linked).
- `linux-x86_64` clang/gcc builds run with **no** `-latomic` flag.
- `windows-x86_64` (mingw gcc, clang, tcc) builds never request libatomic — and with the
  §13.5 defs form primary, they link **zero** external atomic symbols (§14.3).

---

### F3 — Pointer-width atomics  [P1]

**Problem.** The current runtime selects pointer-sized ops via `_Generic`/`UINTPTR_MAX`;
pure Chemical needs an explicit integer pointee of pointer width, and the LLVM backend
currently rejects pointer-to-pointer atomics.

**Design.**

- Library level: `AtomicUsize`/pointer cell type with `usize = if(def.is64Bit) u64 else u32`.
  Pointer atomic **load/store/exchange/CAS** are performed on the pointer type directly;
  pointer **arithmetic RMW** (fetch_add/sub) is expressed in the library as a cast to
  `usize`, perform the op, cast back (LLVM forbids `atomicrmw add` on pointers — this is a
  language-level rule, documented in the lib).
- Compiler level: extend `get_atomic_op_type` (`LLVM.cpp:2873`) to accept pointer pointees
  for `atomic_load/store` and `atomic_op(Xchg)` and both `cmp_exch` variants (LLVM IR allows
  atomics on pointer types); error on arithmetic RMW ops over pointer pointees.
- C level: builtins work on `T*` where `T` is the pointer type directly (CAS on `void*` is
  the classic use). For synthesized paths, the pointee C type renders as `void*`/`char*`.
  The preamble gains a `uintptr_t` typedef (`uint64_t`/`uint32_t` by `is64Bit`) alongside the
  existing self-provided typedefs.

**Acceptance criteria.** An `AtomicUsize`-based Treiber-stack-style push/pop test compiles
and runs on both backends.

---

### F4 — `volatile` qualifier  [P1]

**Problem.** 2c never emits `volatile` (the only reference is avoiding the C keyword as an
identifier, `2cASTVisitor.cpp:392`). Every fallback loop in the deleted C runtime relies on
volatile accesses (`volatile unsigned _chx_mb`, `LONGLONG volatile* dest`, …). Without it,
GCC/Clang at `-O2` may hoist the load out of a spin loop and the loop never terminates.

**Design (annotation-first).**

```chemical
@volatile
var shutdown : u32 = 0        // local/global
```

- Also permitted on struct fields. Not permitted (initially) on: parameters, destructors'
  implicit self, variables of struct type with a `@delete` destructor (error: "volatile is
  only supported on integer, pointer and bool types for now").
- **2c**: the C declaration for the variable/field gains `volatile` (e.g.
  `volatile uint32_t shutdown = 0;`). Derefs through `&raw mut` of a volatile object inherit
  the qualifier naturally via the C type system — no per-access logic needed on the C side.
- **LLVM**: loads/stores of the marked variable/field use volatile loads/stores
  (`CreateLoad(..., /*isVolatile=*/true)`); this is per-access logic in `VarInit` /
  `Identifier` lowering (`ast/statements/VarInit.cpp`, `LLVM.cpp`) — the same files already
  touched by extern-global handling.
- **Interpreter**: no behavioral change (single-threaded semantics).
- **TypeVerify**: no new rules beyond the type restriction above.

Alternative considered and deferred: a volatile *pointer type* (`*volatile T`) gives
finer-grained control but touches the whole type system (parsing, symres, type rewriting,
both backends). The annotation covers the fallback library's needs; the type can come later
without breaking the annotation.

**Acceptance criteria.**
- A pure-Chemical spin loop over a `@volatile` flag, terminated from another thread,
  terminates under gcc/clang `-O2` (this is also the F7 integration test).
- C output inspection shows the qualifier; LLVM IR shows `load volatile`/`store volatile`.

---

### F5 — Compiler-only barrier: `atomic_signal_fence`  [P1]

**Problem.** The universal fallback tool is the compiler-reordering barrier
`__asm__ __volatile__("" ::: "memory")` (supported by GCC, Clang **and** TCC on every
platform — no arch dependency). Today there is no way to emit it from Chemical, and hand
written asm is exactly what we're eliminating.

**Design.**

- New virtual on `BackendContext`:

  ```cpp
  virtual void signal_fence(BackendAtomicMemoryOrder order) = 0;
  ```

- **C backend**:

  | order | emission |
  |---|---|
  | relaxed / monotonic | nothing |
  | acquire / release / acq_rel / seq_cst | `__asm__ __volatile__ ("" : : : "memory")` |

- **LLVM backend**: there is **no distinct "signal fence" instruction in LLVM IR** (an
  earlier draft assumed one — verified false). Clang lowers `__atomic_signal_fence(order)`
  to `IRBuilder::CreateFence(order, llvm::SyncScope::SingleThread)` — a normal `fence`
  instruction constrained to the `singlethread` sync scope, which in-tree targets emit no
  code for. Emit exactly that: `CreateFence(to_llvm_mo(order), llvm::SyncScope::SingleThread)`,
  validating monotonic the same way `atomic_fence` does (or emitting nothing for it).
- **Interpreter/intrinsics**: new compiler-declared function
  `intrinsics::atomic_signal_fence(order)` mirroring `InterpretLLVMAtomicFence`
  (`GlobalFunctions.cpp:~2380`), wired to `backend_context->signal_fence(...)`.
- Library surface: `public comptime func atomic_signal_fence(order : memory_order)` in
  `atomic.ch`.

**Acceptance criteria.** Generated C contains the empty-asm statement where expected; LLVM IR
contains a `fence` instruction with `singlethread` sync scope (LLVM has no distinct
signal-fence instruction — see F5 design); a release-store + signal-fence + acquire-load
litmus test passes under `-O2`.

---

### F6 — Backend capability queries  [P1]

**Problem.** The library must choose, **at comptime**, between the builtin path and the
pure-Chemical fallback path per target — currently the only signal is
`intrinsics::get_backend_name() == "C"`, which is too coarse (a TCC-on-Windows-ARM64 target
needs fallbacks while a Clang-on-Linux C target doesn't).

**Design.**

- Extend `CompilerFeatureKind` (`compiler/lab/BackendContext.h:8`):

  ```cpp
  enum class CompilerFeatureKind : int {
      Float128 = 0,
      AtomicBuiltins   = 1,  // generic __atomic builtins work in the emitted C
      InlineAsm        = 2,  // __asm__ __volatile__ available (signal fence)
      Volatile         = 3,  // volatile qualifier support (F4)
      Last = Volatile,
  };
  ```

- `ToCBackendContext::supports` decides from `target_data` ONLY — arch × OS facts from the
  target triple, never from user compiler input (there is none). The Phase 0 probe fills the
  capability matrix. The capability lattice is **layered** (each level implies the ones
  below): `AtomicBuiltins` (generic `_n` forms accepted) → `SuffixedLibatomic` (`__atomic_*_N`
  symbols + `-latomic`) → `Fallback` (pure-C CAS/word-tear + empty-asm — always true).
  The only compiler-ish fact that can legitimately leak into a C target is `data.tcc`
  (C-backend jobs are always TinyCC-driven), and even that is only a proxy for "may lack
  generic forms" — the real answer comes from the probe (see §12 B).
- `LLVMBackendContext::supports` returns true for all atomic capabilities.
- The existing `intrinsics::supports(...)` comptime call already routes here
  (`GlobalFunctions.cpp:593-598` handles the int-based dispatch) — no new intrinsic needed.
- Ship named constants for the feature ids (e.g. `const AtomicBuiltins = 1` in the library)
  so call sites never use magic numbers like `supports(1)`.
- The atomic library then dispatches capability-first:

  ```chemical
  comptime if(intrinsics::supports(1)) { /* builtin path */ }
  else { /* F7 fallback path */ }
  ```

  with `get_backend_name()` retained only as a last-resort diagnostic.

**Acceptance criteria.** `intrinsics::supports` returns the documented matrix per target;
library dispatch matches it (unit-testable at comptime with `comptime if`).

---

### F7 — Pure-Chemical fallback layer  [P1 — required for the no-common-form corner (Windows-ARM64+tcc); portable everywhere else]

**Problem.** The residual corner: TCC on Windows-ARM64 (and any target where the probe shows
missing builtins) has neither generic `__atomic` builtins nor libatomic. The deleted
`win/atomic.h` solves this with CAS loops, word-tearing emulation and inline asm; we must
reproduce it **in Chemical** so the compiler generates equivalent C.

**Design.** New module files under `lang/libs/atomic/src/fallback/`:

- `cas.ch` — CAS-loop RMW synthesis for all five bitwise/arithmetic ops over a type that has
  a working CAS:

  ```chemical
  @inline
  public func fallback_fetch_or_u32(x : %runtime<*mut u32>, v : %maybe_runtime<u32>, order : memory_order) : u32 {
      var old : u32 = atomic_load_u32(x, memory_order.relaxed)
      while(true) {   // RUNTIME loop — emitted into the generated C, NOT evaluated at comptime
          if(atomic_compare_exchange_weak_u32(x, &raw mut old, old | v, order, fallback_failure_order(order))) {
              return old
          }
      }
  }
  ```

  (Implementation note: the fallbacks are **ordinary runtime functions** — `@inline` functions
  whose bodies contain runtime loops. The comptime system only decides *whether* a caller
  dispatches to the fallback or the builtin path; it never evaluates the loop. Do NOT mark
  the loop `comptime` — that would run it during compilation instead of emitting it.)

- `wordtear.ch` — 8/16-bit and 64-bit emulation over a working 32-bit (or 64-bit) CAS,
  ported from `ManualInterlockedExchange8/16`, `ManualInterlockedCompareExchange8/16`,
  `ManualInterlockedExchangeAdd16`, `ManualInterlockedOr/Xor/And{16,32,64}`:
  - compute containing-word address: `word = ptr as usize & ~3`, `shift = (ptr as usize & 3) * 8`,
    `mask = 0xFFFF << shift` (pure arithmetic — no compiler support needed),
  - read–modify–CAS loop on the aligned word,
  - this code *requires* F4 (volatile on the containing word access) and F5 (signal fences
    between read and CAS) to be correct under optimization.

- `fence.ch` — fence synthesis for targets with no hardware fence intrinsic
  (Windows-ARM64+tcc). **Be honest about what the current header does there**: the
  `MemoryBarrier` shim in `win/atomic.h:41-46` is a **compiler-only barrier** (a volatile
  write — it issues no CPU memory-ordering instruction), because TCC lacks
  `__atomic_thread_fence` on ARM and the SDK `MemoryBarrier` intrinsic is MSVC-only. So on
  that corner today, acquire/release fences are already partially broken (compiler
  reordering is prevented, hardware reordering is not). F7's baseline obligation is to
  **match** current behavior: empty-asm compiler barriers (F5). Upgrading to a real hardware
  fence on that corner requires non-empty inline asm (`dmb ish` / `dmb ishst`) — whether TCC
  accepts non-empty inline asm on ARM/ARM64 is an **open probe question (§12 E.6)**; if yes,
  emit it (gated only by target facts); if no, document the corner as degraded
  (hardware reordering unsynchronized, exactly as today). Do not claim full hardware
  ordering on that corner without the probe.

- Dispatch: chosen by F6 capability queries plus `def.*` arch checks (`def.arm`,
  `def.aarch64`, `def.x86_64`).

- `cpu_relax` — **optional** `intrinsics::cpu_relax()` (`BackendContext::cpu_relax`): x86
  `__builtin_ia32_pause`/`pause` asm on GCC/Clang; TCC-on-ARM no-ops (current header already
  no-ops there). Purely a power/perf nicety; spin loops are correct without it. Ship last or
  never.

**Acceptance criteria.**
- On `windows-arm64-tcc` (real or CI-emulated): the full atomic test suite passes using only
  fallback paths; no unresolved `__atomic_*` symbols in the object.
- The same fallback code compiles on linux (forced via a build arg) and passes the suite —
  proving it's correct, not just present.

---

### F8 — Interpretation-mode backend context  [P2]

**Problem.** `do_interpretation_job` (`LabBuildCompiler.cpp:4797`) creates a
`GlobalInterpretScope` but never assigns `global.backend_context` (the TCC paths assign at
`:1759`, `:3306`; LLVM at `:2023`). Any atomic intrinsic called during an interpretation run
(`--arg-interpret`, LSP hover evaluation of comptime code) dereferences garbage.

**Design.**

- New `SequentialBackendContext : BackendContext` (name: `"Interpret"`):
  `atomic_load` → dereference (interpreter pointer semantics), `atomic_store` → assignment,
  `atomic_cmp_exch_*` → compare + conditional store, `atomic_op` → compute + store,
  `atomic_fence`/`signal_fence` → no-op. Single-threaded semantics are trivially correct and
  make the whole library testable under `--arg-interpret`.
- Also give `GlobalInterpretScope` a default null-object context so an unassigned context
  degrades to a clean "not supported in this context" error instead of UB.

**Acceptance criteria.** `./chemical lang/tests/build.lab --arg-interpret` runs the new
atomic tests (§8) without crashing and with sequential semantics.

---

### F9 — Intrinsic declaration hygiene  [P0, trivial]

Listed fully in §1.3: fix `atomic_op`'s declared return type, the `expParam` index, the
`*mut void` typo (moot after the rewrite but fix the intrinsic side regardless), and decide
the `intrinsics::atomic_*` rename. Fold into the F1 PR.

---

## 4. The rewritten atomic library

### 4.1 Layout (end state)

```
lang/libs/atomic/
├── chemical.mod              # or a slimmed build.lab — no c_file_module, conditional link only
├── src/
│   ├── atomic.ch             # public API — ONE unified comptime path, capability-dispatched
│   ├── intrinsics_bridge.ch  # thin wrappers: order mapping + intrinsics::llvm::atomic_* calls
│   ├── fallback/
│   │   ├── cas.ch            # F7 CAS-loop RMW synthesis
│   │   ├── wordtear.ch       # F7 8/16/64-bit emulation over 32-bit CAS
│   │   └── fence.ch          # F7 fence synthesis for no-fence targets
│   └── types/
│       ├── atomic_u8.ch      # unchanged public wrappers
│       ├── atomic_u16.ch
│       ├── atomic_u32.ch
│       ├── atomic_u64.ch
│       └── atomic_usize.ch   # new (F3), optional
```

Deleted: **all of `cruntime/`** (≈2500 lines of C), the 44 `@extern` declarations, the
`get_backend_name() == "C"` branches.

### 4.2 Unified API shape

```chemical
public comptime func atomic_load_u64(x : %runtime<*u64>, order : memory_order = memory_order.seq_cst) : u64 {
    comptime if(intrinsics::supports(CompilerFeature.AtomicBuiltins)) {
        return bridge::load_u64(x, llvm_mem_order(order))
    } else {
        return fallback::load_u64(x, order)
    }
}
```

Public signatures stay **byte-for-byte compatible** with today's API (same names, params,
defaults) — `std`, `net`, `http` and everything else importing `atomic` keeps working
unchanged. (`grep` shows no in-tree importer today besides tests-to-be — the API freeze is
for external users.)

### 4.3 Build wiring

- `build.lab`: delete the `c_file_module` block; add the conditional `link_system_lib`
  from F2. The `user_job.getTarget().c` guard remains (LLVM jobs need no link flag).
- No resource shipping, no object caching changes.

---

## 5. Generated-C reference (golden examples)

Exact C the C backend must produce after the plan (target: linux-x86_64, clang/gcc or tcc
with builtins; `__ATOMIC_*` literals shown symbolically, emitted numerically):

```chemical
// chemical
var x : u64 = 0
var expected : u64 = 0
atomic_store_u64(&raw mut x, 42, memory_order.seq_cst)
var v  = atomic_load_u64(&raw mut x, memory_order.acquire)
var ok = atomic_compare_exchange_strong_u64(&raw mut x, &raw mut expected, 7, memory_order.seq_cst, memory_order.acquire)
var o  = atomic_fetch_or_u64(&raw mut x, 0xFF, memory_order.acq_rel)
atomic_fence(memory_order.seq_cst)
```

```c
/* generated C (bodies only) */
__atomic_store_n((&x), 42, 5);
uint64_t v = __atomic_load_n((&x), 2);
_Bool ok = __atomic_compare_exchange_n((&x), (&expected), 7, 0, 5, 2);
/* fetch_or has no builtin → synthesized statement expression */
uint64_t o = (*({
    uint64_t __chx__old = __atomic_load_n((&x), 0);
    for(;;) {
        if(__atomic_compare_exchange_n((&x), &__chx__old, (uint64_t)(__chx__old | 0xFF), 1, 4, 2)) break;
    }
    __chx__old;
}));
__atomic_thread_fence(5);
/* signal fence (F5), where emitted */
__asm__ __volatile__ ("" : : : "memory");
```

Windows-x86_64-tcc (if the probe shows the `_n` forms missing): identical structure with
size-suffixed symbols:

```c
__atomic_store_8((&x), 42, 5);
uint64_t v = __atomic_load_8((&x), 2);
_Bool ok = __atomic_compare_exchange_8((&x), (&expected), 7, 0, 5, 2);
uint64_t o = (*({
    uint64_t __chx__old = __atomic_load_8((&x), 0);
    for(;;) {
        if(__atomic_compare_exchange_8((&x), &__chx__old, (uint64_t)(__chx__old | 0xFF), 1, 4, 2)) break;
    }
    __chx__old;
}));
```

Windows-arm64-tcc (fallback path, F7): no `__atomic_*` symbols at all — CAS over volatile
32-bit words, word-tearing emulation, synthesized fences. CAS for u64 on this target:

```c
uint64_t atomic_cas64_fallback(volatile uint32_t* lo_ptr, volatile uint32_t* hi_ptr,
                               uint64_t expected, uint64_t desired);
/* — emitted from pure Chemical: two 32-bit CAS steps with comparator reconstruction.
   lo/hi derivation is endianness-dependent: comptime-select via def.little_endian /
   def.big_endian (both exist on TargetData). — */
```

---

## 6. Platform / compiler matrix (what each target gets)

One row per target configuration = **one emitted C artifact, compilable by gcc, clang and
tcc alike**. The matrix lists what the single emitted C uses — never per-compiler variants
(see §F1.5). The "linking" column is our driver's decision and does not change the C.

| Target | load/store/RMW/CAS | fence | linking (our driver) | fallback needed |
|---|---|---|---|---|
| linux/bsd x86_64, aarch64 | generic builtins (all 3 compilers compile them) | `__atomic_thread_fence` | `-latomic` only for tcc-driver jobs | no |
| linux/bsd i386, arm32 | generic builtins | builtin | `-latomic` always | no |
| macos x86_64, aarch64 | generic builtins | builtin | never `-latomic` (not shipped on macOS) | tcc-on-macos per probe (maybe) |
| windows x86/x64 | **§13.5 defs form** (Windows amendment §14.2); F7 behind it | **synthesized** dummy-cell exchange — never `__atomic_thread_fence` (§13.1) and no libatomic C11 name on Windows | none — **zero external deps** | probably not |
| windows arm64 | **F7 fallback** behind the §13.5 preamble defs | **F7 synthesis** (compiler barriers; hardware fence only if the §12 E.6 probe passes) | none — **zero external deps** | **yes** |
| any + interpretation | sequential semantics (F8) | no-op | — | — |

Out of scope (documented non-goals): MSVC as a C compiler for 2c output; cygwin as a C
compiler for 2c output (`def.cygwin` exists — targeting it is a non-goal); float atomics on
the C backend; `UIncWrap`/`UDecWrap`/`F*` RMW ops on the C backend; `consume` as a distinct
ordering (== acquire, as LLVM models it).

---

## 7. Implementation phases & dependency DAG

```
P0 probes/baseline ──┬──> P1 F1+F9 (C intrinsics core) ──> P2 F2 + library rewrite ──> M2 ✅ linux/macos/win-x86
                     │                                          │
                     └──> P3 F4 (volatile) ──┬──> P4 F7 (fallback layer) ──> M4 ✅ windows-arm64-tcc
                                             │        ▲
                       P3 F5 (signal fence) ─┤────────┘
                       P3 F6 (capabilities) ─┘
P3 F3 (pointer atomics) ──> P5 (usize type + polish)
P5 F8 (interpretation backend) — independent, anytime after P1
```

### P0 — Probes & baseline (no compiler changes)

1. **TCC builtin probe matrix.** For the libtcc we bundle (and mob HEAD): compile a C file
   using every `__atomic_*_n` generic form and every size-suffixed form; run under
   `linux-x86_64`, `linux-aarch64`, `windows-x86_64` (mingw cross), `windows-arm64` (if
   available). Record: which forms compile, which link, which produce inline code. Also
   verify `__atomic_thread_fence` and the empty-asm statement. Output: a table committed to
   this doc (appendix) driving F1.5/F6.
2. **Verify the 5-arg `__atomic_compare_exchange_N` suspicion** (§1.3) with a tiny
   valgrind/gdb experiment — for the record.
3. **Baseline test suite**: create `lang/tests/libs/atomic/` (`chemical.mod` + `src/`) with
   sequential single-thread tests for every public op × every width × every order (see §8).
   These must pass on the **LLVM** backend immediately (it works) and become the target for
   the C backend.
4. **Golden-C harness**: `./scripts/test.sh --tcc --emit-c` snapshot of the isolated
   `temp.ch` test, plus a script compiling the snapshot with gcc/clang/tcc and running it.

### P1 — F1 + F9 (the C backend core)

Implement per §F1: order mapping, `render_value`, the six intrinsics (four direct, CAS,
op-matrix with synthesis), intrinsic declaration fixes. Land the golden-C tests. Gate: all
P0 tests pass under `--tcc` on linux-x86_64 with gcc/clang/tcc compiled output.

### P2 — F2 + library rewrite

Conditional libatomic linking; rewrite `src/atomic.ch` to the unified capability-dispatched
path (with F6 stubbed as "builtins always for now, fallback TODO"); **delete `cruntime/`**;
update `build.lab`. Gate: full `--tcc` and `--llvm` suites green; `--libs` green; grep proves
zero C files remain in the library.

### P3 — Language features (F4, F5, F6, F3)

Independent PRs: `@volatile`; `signal_fence` intrinsic; capability queries; pointer atomics
+ `AtomicUsize`. Each with its own acceptance criteria from §3.

### P4 — F7 fallback layer

Port the fallbacks to pure Chemical, wire capability dispatch, and prove on the Windows
targets. This is the largest remaining chunk and the only one gated on real Windows-ARM64
testing.

### P5 — F8 + polish

Interpretation-mode backend context; `intrinsics::atomic_*` rename; docs
(`AGENTS.md` gotchas entry: "atomic intrinsics — backend contract" and the volatile
annotation); changelog; CI additions (atomic smoke job on the cross-compiler matrix).

---

## 8. Testing strategy

### 8.1 Unit tests (`lang/tests/libs/atomic/`)

Standalone module (`chemical.mod` + `src/main.ch`) following the libs-suite conventions
(run via `./scripts/test.sh --tcc --libs` / `--test-plugins` style dispatch):

- **Sequential correctness** (all backends): for each width (u8/u16/u32/u64/usize) × each op
  (load, store, exchange, fetch_add/sub/and/or/xor, CAS weak/strong) × a representative order
  set (relaxed, acq_rel, seq_cst): start value, apply op, assert old/new values. CAS tests
  must assert the **expected-out** side effect (old value written back).
- **Weak CAS tolerance**: loop weak CAS until success, assert eventual success and at most-N
  bound generosity.
- **Order acceptance**: every `memory_order` variant is accepted by every op (no crash, sane
  values).
- **Concurrency** (optional, timeboxed): two `std::thread` workers doing fetch_add on one
  counter, join, assert total; a volatile-flag shutdown loop (F4 integration). Skip
  gracefully where threads are unavailable.
- **Negative tests** (`lang/tests/negative/` additions): fence with monotonic order (compile
  error, mirroring LLVM); CAS failure order stronger than success; atomic op on a
  non-integer pointee; `@volatile` on a destructor-bearing struct.

### 8.2 Golden-C tests

- Isolated `lang/compiled/temp_atomic.ch` exercised via the standard isolation workflow.
- **Windows amendment (§14.8)**: on Windows the "every available host compiler" set is
  MSYS2 mingw-w64 gcc (record ucrt vs msvcrt CRT), clang for Windows, and the bundled
  `lib/tcc/tcc.exe` — see §14.8 for the exact matrix and mingw warning-clean notes.
- Snapshot the `--emit-c` output; a test script compiles the snapshot with every available
  host compiler (`gcc`, `clang`, `tcc`) at `-O0` and `-O2` and runs it — same results.

### 8.3 Backend parity

- The same `.ch` test source must produce identical observable behavior under `--tcc` and
  `--llvm` (and `--arg-interpret` after F8, with sequential semantics).

---

## 9. Risks & mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| TCC builtin coverage differs by version/target (R1) | Wrong spelling → link errors on some targets | P0 probe matrix pins reality for the bundled libtcc; F1.5 makes spelling target-derived; F6 lets the library fall back |
| Windows-ARM64 + TCC needs the full fallback layer (R2) | Biggest single chunk of work | F7 is self-contained Chemical; can ship M2 (everything except that corner) first; fallback provable on linux by forcing the dispatch |
| `render_value` hits values that can't render in expression position (R3) | Compiler crashes or wrong C | Explicit diagnostic + test matrix of arg shapes (plain var, field, deref, call result rejected) |
| `@volatile` semantics drift between backends (R4) | Subtle heisenbugs | Restrict annotation to integers/pointers/bools; parity tests in §8.3; LLVM volatile loads/stores tested via IR inspection |
| Synthesized statement expressions interact badly with sret/struct contexts (R5) | Nested `(*({…}))` breakage | RMW synthesis only for integer pointees (no sret involved); golden tests inside struct-returning callers |
| libatomic absence on unusual posix (musl/Alpine) (R6) | Link failure for tcc/i386 | musl ships libatomic; Alpine needs `libatomic` package — document as dependency for tcc/i386 targets; everything else links nothing |
| Comptime `while` inside emission-capability branches confuses caching (R7) | Stale C between target flips | Capability branches evaluated per job like existing `def.*` branches (mechanism already proven by `def.windows` usage in libs) |
| LLP64 typing trap on Windows (R8) | Preamble defs typed with `long`/`unsigned long` mismatch gcc's per-size builtins on Win64 → poisoned builtins (§13.3 probe F/G failure mode) | Defs declared **only** against the preamble's self-provided fixed-width typedefs (`uint64_t` from `is64Bit`/`win64`); never `long`/`unsigned long`. Verified by the §14.8 harness at `-Wall -Wextra` |

---

## 10. Definition of done

- [ ] `lang/libs/atomic` contains **zero** `.c`/`.h` files; `cruntime/` deleted.
- [ ] `build.lab` (or `chemical.mod`) has no `c_file_module`; conditional `-latomic` only on
      posix targets that need it — **never any link flag on Windows targets** (§14.3).
- [ ] All six intrinsics implemented in `ToCBackendContext` with target-aware spelling;
      LLVM backend bit-identical behavior to today.
- [ ] Full sequential test matrix passes under `--tcc`, `--llvm` (and `--arg-interpret`
      after F8) on linux-x86_64.
- [ ] Golden C snapshots compile & run identically under gcc, clang and tcc at `-O0`/`-O2`.
- [ ] **Golden C snapshots compile & run identically on Windows** under mingw gcc, clang
      and bundled `lib/tcc/tcc.exe` at `-O0`/`-O2`, warning-clean per §14.8.
- [ ] Compiler-independence verified: the same target always yields one C artifact; no
      compiler flag influences codegen; every emitted construct is in the gcc/clang/tcc
      intersection for that target (§12 C checklist).
- [ ] **LLP64 verified**: no emitted def or signature assumes `long` is 8 bytes; the
      fixed-width typedef set is the only spelling used (§14.2).
- [ ] `@volatile`, `atomic_signal_fence`, capability queries, pointer atomics landed with
      tests.
- [ ] Windows-x86_64 passes with gcc/clang/tcc with **zero external atomic dependencies**
      (no `libatomic-1.dll`, no import libs); Windows-ARM64 passes via the pure-Chemical
      fallback layer (or the corner is explicitly documented as the single remaining gap).
- [ ] Public API of the library unchanged; no `#ifdef` and no C macros in generated C beyond
      the pre-existing preamble guards.
- [ ] `AGENTS.md` and skills updated (atomic intrinsics contract, volatile annotation,
      capability queries).

---

## 11. Appendix — evidence index

| Fact | Reference |
|---|---|
| BackendContext contract & comptime-codegen intent | `compiler/lab/BackendContext.h:21-86` |
| C-backend atomic stubs (the blocker) | `preprocess/2c/2cBackendContext.h:47-82` |
| LLVM atomic implementations | `compiler/backend/LLVM.cpp:2896-2974`; `get_atomic_op_type` `:2873` |
| Atomic op/order/scope enums (LLVM numbering) | `compiler/lab/backend/atomics.h` |
| Comptime intrinsic classes & dispatch | `ast/utils/GlobalFunctions.cpp:2380-2700`; namespace `:2690-2715` |
| `get_backend_name` / `emit` / `raw` | `GlobalFunctions.cpp:1999, 2038, 2049-2085` |
| Capability query precedent (`supports`) | `GlobalFunctions.cpp:593-598` |
| `RawLiteral` verbatim C rendering | `ast/values/RawLiteral.h`; `2cASTVisitor.cpp:7152` |
| C-side Value-rendering precedent (`mem_copy`) | `2cASTVisitor.cpp:7947-7953` |
| Self-contained C preamble, target-aware typedefs | `2cASTVisitor.cpp:3219+` |
| Statement-expression precedent (sret) | `2cASTVisitor.cpp` sret pattern; TCC support proven in-tree |
| Backend context wiring per job | `LabBuildCompiler.cpp:1758-1759, 2021-2023, 3303-3306` |
| Interpretation job lacks backend context | `LabBuildCompiler.cpp:4797+` |
| `def.*` target values | `GlobalFunctions.cpp:3166-3187`; `LabJob.cpp` `is_condition_enabled` |
| Conditional `link` in module files | `parser/statements/LexStatement.cpp:630-664`; `ModToLabConverter.cpp:305-332` |
| `comptime if` inside build.lab precedent | `lang/libs/tls/build.lab:31` |
| `link_system_lib` CBI binding | `compiler/cbi/bindings/BuildContextCBI.cpp:51` |
| TCC→libatomic direct symbols (no generic builtins comment) | `lang/libs/atomic/cruntime/nix/atomic.h` header comment + declarations |
| TCC accepts `__atomic_compare_exchange` on Windows x86 | `lang/libs/atomic/cruntime/win/atomic.h:33-47` |
| Word-tearing emulation source to port | `win/atomic.h` `ManualInterlocked{Exchange,CompareExchange,ExchangeAdd}{8,16}` + `ManualInterlocked{Or,Xor,And}{16,32,64}` |
| Volatile-dependent fallback patterns | `win/atomic.h:41-46` (MemoryBarrier shim), `ManualInterlocked*` loops |
| `cpu_relax` arch matrix | `nix/atomic.h` (pause/yield/fence/or/ssnop/nop), `win/atomic.h:61-67` |
| Library has no tests today | absence of `lang/tests/libs/atomic/` |
| Suspected 5-arg CAS ABI bug | `nix/atomic.h` `__atomic_compare_exchange_{4,8}` declarations |
| `atomic_fetch_sub_explicit` mis-mapping | `nix/atomic.h` `#define atomic_fetch_sub_explicit __atomic_sub_fetch_4` |
| Intrinsic decl bugs | `GlobalFunctions.cpp:2608-2611` (param index), `~2631` (void return), `atomic.ch` `*mut void` u16 store |

---

## 12. Appendix — clarifications & verification checklist for the implementing agent

This section exists so an implementer never has to guess. If any decision below conflicts
with what you find during implementation, update this doc **in the same PR** — never decide
silently.

### A. Emission-time model (the most important non-obvious fact)

The atomic intrinsics are **comptime functions interpreted at comptime, but called from
runtime (emitted) code**. During codegen of a runtime function, the interpreter evaluates the
comptime function's body; when it reaches `intrinsics::llvm::atomic_load(...)`, the intrinsic
dispatches to `BackendContext`, and the C backend renders into the output *at the current
position*. Practical consequences:

- The C backend's atomic methods run mid-statement, at expression position. Anything that
  would need to emit *statements* (constructor temporaries, sret calls) is invalid there —
  hence the `render_value` constraints (F1.2) and its rejection diagnostics.
- A `RawLiteral` returned from the intrinsic flows back through the interpreter as the
  comptime call's value and is rendered later when the visitor reaches the call node
  (`2cASTVisitor.cpp:7152`). The returned value's Chemical type (`getType()`) MUST be set
  correctly — it drives the C declaration of `var x = atomic_load_u64(...)` — not just the
  string content.
- Order/scope/op arguments arrive as comptime integers (already validated by the intrinsic
  wrappers). Render them as decimal literals in the C.
- The same mechanism serves the LLVM backend today; any change to the intrinsic classes
  (`GlobalFunctions.cpp`) must keep `LLVMBackendContext` behavior byte-identical.

### B. What `def.tcc` / `def.clang` / `def.c` actually mean (do not over-read them)

- `def.clang` is true for the LLVM/Clang **product build** (`Compiler` binary,
  `COMPILER_BUILD` — `TargetData.h:75-79`). It does NOT mean "the user will compile the
  emitted C with clang".
- `def.tcc` is true for C-backend jobs (TCCCompiler, CBI, JIT —
  `LabBuildContext.cpp:24-46`). For C targets it happens to mean "the C backend is driving
  this job" — the only compiler-ish fact that legitimately leaks into a C target, and only
  for *driver/link* decisions (F2), never for codegen spelling (F1.5).
- `def.c` is true for any job generating C. Use it for "generated C exists", never for
  compiler-variant decisions.
- Platform facts (`def.windows`, `def.posix`, `def.x86_64`, `def.i386`, `def.aarch64`,
  `def.arm`, `def.musl`, …) come from the target triple (`prepare_target_data`,
  `GlobalFunctions.cpp:3020+`, `TargetData.h`) — fully reliable, use freely.
- **Windows note (§14.2)**: Windows is LLP64 — `long` and `unsigned long` are **4 bytes**
  on win64, unlike LP64 posix hosts. Any preamble definition or signature spelling must use
  the self-provided fixed-width typedefs (`uint64_t` etc., emitted by `prepare_translate()`
  from `is64Bit`/`win64`), never `long`/`unsigned long`. The §13.3 per-size typing
  observations are LP64 probe data; re-verify on Windows per §13.10.

### C. Universal-construct checklist (anything we emit must be in this intersection)

| Construct | gcc | clang | tcc | Notes |
|---|---|---|---|---|
| `__atomic_*_n` generic builtins (call sites) | ✅ | ✅ | ❌ **resolved by P0**: tcc does not know them at all (parse warns, link fails) — §13.1 | emit as call sites **only** with pure-C preamble defs (§14.2) |
| pure-C preamble defs of `__atomic_*_n` names (§13.5) | ✅ accepted, dead code | ✅ accepted, dead code | ✅ resolves calls to defs | **primary form** — Windows amendment §14.2 |
| `__atomic_*_N` size-suffixed libatomic symbols | ✅ (builtin, inlines with exact sigs) | ✅ (calls → `-latomic`) | ✅ (calls → `-latomic`) | **posix fallback** — §13.4; **never on Windows** (no libatomic, §14.1) |
| `__atomic_thread_fence(n)` | ✅ | ✅ | ❌ **resolved by P0**: unresolved under tcc — §13.4 | synthesize via dummy-cell exchange — the only fence form on Windows (§14.7) |
| `__asm__ __volatile__("" ::: "memory")` | ✅ | ✅ | ✅ (proven: used by `win/atomic.h` compiled under tcc) | signal fence (F5) |
| GNU statement expressions `({ ... })` | ✅ | ✅ | ✅ (P0-probed: value/pointer/nested/discarded all pass — §13.6) | RMW synthesis (F1.4) |
| `volatile` qualifier in declarations | ✅ | ✅ | ✅ | F4 |
| C11 `_Atomic` types | ✅ | ✅ | ❌ | **never emit** |
| Compiler-specific spellings (`_mm_pause`, `__sync_*`, `Interlocked*`) | partial | partial | ❌ | **never emit** |

### D. Per-PR verification checklist

1. **Target-only determinism**: assert the new codegen paths read only `target_data` (plus
   mode), never a compiler selection — same target ⇒ byte-identical C.
2. **All-compilers check**: compile the emitted C with gcc, clang AND tcc at `-O0` and
   `-O2`; run all; identical outputs.
3. **Warning-clean**: `-Wall -Wextra` clean on gcc/clang for the atomic emission (no
   unused-value warnings from statement expressions in discarded contexts — `(void)`-cast
   where the result is discarded).
4. **LLVM parity**: `llvm_ir.ll` snapshots for the atomic tests must show zero diff from the
   F1 PR.
5. **Interpretation safety**: before F8 lands, intrinsic dispatch must error cleanly on a
   null `backend_context` (no crash); after F8, `--arg-interpret` runs with sequential
   semantics.
6. **No stubbed virtuals**: every new `BackendContext` virtual is implemented in BOTH
   backends and the sequential context (F8).
7. **Output purity**: no new `#include` in emitted C; no `#ifdef` beyond the pre-existing
   preamble guards; no C macros emitted.
8. **LLP64 check (Windows)**: no emitted def or signature spells an 8-byte type as `long`
   or `unsigned long`; only the self-provided fixed-width typedef set appears (§14.2).
9. **Windows zero-deps check**: emitted C for Windows targets contains no `libatomic`
   reference, no `-latomic` link flag, and no `Interlocked*`/`MemoryBarrier` externs
   (§14.1, §14.6).
10. **Windows warning-clean**: `-Wall -Wextra` silent on mingw gcc and clang for Windows
    too; mingw emits `-Wattributes`/dllimport/decorated-name noise the linux harness
    doesn't — define the baseline per §14.8.

### E. Open questions the implementer must resolve via the P0 probe (do not guess)

1. Which `__atomic_*_n` generic forms does the bundled libtcc accept, per target
   (linux-x86_64, linux-aarch64, windows-x86_64, windows-arm64, macos)?
2. Does tcc accept `__atomic_thread_fence` on each target?
3. Does tcc-on-Windows link/inline the builtins, or emit undefined `__atomic_*_N` calls
   (needing F7 since there is no libatomic on Windows)?
4. Does tcc-on-macos accept the builtins (decides the macos fallback question)?
5. Confirm the 5-arg `__atomic_compare_exchange_N` ABI suspicion (§1.3) — for the record
   only; the code we delete is wrong regardless.
6. Does TCC accept **non-empty** inline asm on ARM/ARM64 (e.g.
   `__asm__ __volatile__("dmb ish" ::: "memory")`)? Empty-asm is proven (§12 C); non-empty
   asm on ARM decides whether F7 can emit real hardware fences on the Windows-ARM64 corner
   (see F7 `fence.ch`) or must degrade to compiler-only barriers. (x86 evidence is positive:
   `win/atomic.h` emits `mfence` inline asm compiled by tcc.)

Record probe results as a new appendix table in this doc before starting P1.

> ✅ **DONE — results recorded in §13** (linux-x86_64 host; gcc 15.2.0, clang 21.1.8, tcc
> 0.9.28rc mob@2ba12e8). §12 E.1/E.2/E.5 are resolved for linux-x86_64; E.3/E.4/E.6 remain
> for cross-targets (windows, macos, ARM asm).

---

## 13. Appendix — P0 probe results (EXECUTED, linux-x86_64 host, 2026-09-08)

Probes live in `lang/compiled/atomic_probe/src/` (gitignored area). Compilers tested:
gcc 15.2.0, clang 21.1.8, **tcc 0.9.28rc mob@2ba12e8 (2026-08-09)** — the exact libtcc we
bundle (`lib/tcc`). Every "✅ pass" row below means: compiled, linked, ran, and all
self-verification checks returned 0. Cross-target probes (mingw, aarch64) still TODO —
see §13.9.

### 13.1 The generic `_n` builtins: TCC does not support them — CONFIRMED

`src/generic_builtins.c` (all `__atomic_load_n/store_n/exchange_n/compare_exchange_n/fetch_*`
forms, all widths, plus fences and asm):

| Compiler | Result |
|---|---|
| gcc -O0/-O2 | ✅ compile (zero warnings), run, all checks pass. **Zero libatomic calls at -O2.** |
| clang -O0/-O2 | ✅ compile, run, all checks pass. **Zero libatomic calls at -O2.** |
| tcc | ❌ **Every generic form: implicit-declaration warning at parse, then `unresolved reference` at link** (`__atomic_store_n`, `__atomic_load_n`, `__atomic_exchange_n`, `__atomic_compare_exchange_n`, `__atomic_thread_fence`, all `fetch_*`). tcc merely treats these as ordinary undefined function names. |

The `nix/atomic.h` comment ("tcc is not capable to use generic C functions") is **correct**.
The `win/atomic.h` counter-evidence (`__atomic_compare_exchange` under tcc) refers to the
**multi-operand no-suffix family**, which tcc parses but with **divergent semantics** — see
§13.2. **F1's emission matrix must therefore never emit `_n` forms**: gcc/clang inline
them, tcc breaks. The doc's F1.4 emission table is amended by §13.4 below.

**Windows follow-up (§14.4)**: since tcc resolves `__atomic_*` names as plain undefined
externs, and `win/atomic.h:39-47` calls `__atomic_compare_exchange`, the current
`cruntime/` path plausibly **fails to link under Windows tcc** (no libatomic there).
Probe §13.10 P9 settles this; if confirmed, §1 should be amended to note that Windows is
broken today, making this plan a fix rather than a nice-to-have (see §14.4).

### 13.2 The multi-operand no-suffix family (`__atomic_load/store/exchange/compare_exchange`): unusable

`src/multiop_builtins.c`: tcc parses these but **requires pointer temporaries for every
value argument** (`__atomic_store(&x8, &val, 5)` — "pointer expected" for `__atomic_store(&x8, 42, 5)`),
and — decisive — **returns pointers** for the fetch family (`__atomic_fetch_add` returning
a pointer; "assignment makes integer from pointer"). The value-arg signature of
`__atomic_exchange` also differs between gcc (value) and tcc (pointer). There is **no
single spelling** of this family that works on all three compilers. **Never emit it.**

### 13.3 The suffixed family `__atomic_*_N`: THE universal form

`src/suffixed_unified.c` (exact per-size declarations; 8-byte type = `unsigned long` on
LP64): one artifact, all compilers, all checks pass:

| Compiler | Builtins recognized? | Link flags | Result |
|---|---|---|---|
| gcc | **yes, exact per-size signatures** (value args typed per width, `_8` value = `unsigned long` on LP64) | **none needed** — everything inlined (0 `call __atomic` at -O2); with `-latomic` also fine | ✅ |
| clang | **no** — always emits symbol calls (generic-typed only: `_1/_2/_4` return `u32`, `_8` returns `u64`/`unsigned long long`) | **`-latomic` required** | ✅ |
| tcc | **no** — always emits symbol calls (natural-width value types work; generic `size_t`-typed value args also work for `_N` store) | **`-latomic` required** | ✅ |

**Critical ABI detail discovered:** the suffixed symbols have **per-compiler builtin
typing**. gcc's builtin variants are typed per size (load_1 returns `u8`, load_2 returns
`u16`, load_4 returns `u32`, load_8 returns `unsigned long`); clang/tcc (libatomic) export
the SAME symbol names with **generic `u32`-returning** signatures for `_1/_2/_4` (libatomic
returns `u32`/`unsigned long` internally). Consequence: **any preamble declaring these
names must use exact per-size signatures matching each compiler's expectation** — but you
cannot satisfy gcc's exact-typed builtin and clang's generic-typed builtin with one
declaration if they disagree. THEY DO NOT DISAGREE for the forms we need: the §13.5
resolution (generic calls + pure-C defs) makes the declarations moot for gcc/clang
(inline) and only tcc consumes them (any consistent ABI works). Alternatively the
per-size-typed declarations from `src/suffixed_unified.c` work on all three (gcc inlines,
clang/tcc link against libatomic whose ABI is compatible at call sites we emit).
Probe F/G evidence: mismatched declarations poison gcc (it disables builtins and emits
calls → needs `-latomic` too) and poison clang with `-Wbuiltin-declaration-mismatch`
warnings. **Exact signatures are mandatory.**

> ⚠️ **Windows/LLP64 warning (§14.2)**: the `unsigned long` observations above are **LP64
> (linux) probe data**. On Windows, `unsigned long` is **4 bytes** — gcc's `_8` builtin
> there is typed against `uint64_t`-equivalent types, not `unsigned long`. Any preamble
> def or declaration must therefore spell 8-byte types with the self-provided fixed-width
> typedefs (`uint64_t` from `is64Bit`/`win64`), never `long`/`unsigned long`. The
> per-size typing must be re-verified on Windows per §13.10 before relying on it.

### 13.4 AMENDED F1 emission table (supersedes §F1.4 rows)

Probes change the per-op emission as follows — **one spelling per target, all compilers**:

| Op | Emitted C (all compilers) | Notes |
|---|---|---|
| load/store/exchange/CAS (1/2/4/8-byte int) | **suffixed `__atomic_load_N` / `__atomic_store_N` / `__atomic_exchange_N` / `__atomic_compare_exchange_N`** (6-arg CAS with `weak` arg) + exact per-size preamble declarations | gcc inlines; clang/tcc call libatomic (posix: `-latomic` by our driver) — OR use the §13.5 defs and never link libatomic anywhere |
| RMW add/sub/and/or/xor/nand | **suffixed `__atomic_fetch_{add,sub,and,or,xor,nand}_N`** | same linkage story; libatomic exports all of these incl. `nand` (`nm` verified: `__atomic_fetch_nand_{1,2,4,8}`) |
| max/min/umax/umin | **no libatomic symbols exist** (`nm` verified — no `*max*`/`*min*` exports) | keep F1.4 statement-expression synthesis, but built on suffixed CAS + suffixed relaxed load |
| thread fence | **synthesized**: `__atomic_exchange_{1,N}(&__chx__fence_cell, 0, mo)` on a static volatile dummy cell (`src/fence_synth.c` — all 3 compilers ✅) | `__atomic_thread_fence` builtin: gcc/clang yes, **tcc NO** (unresolved); libatomic exports only C11-named `atomic_thread_fence` |
| signal fence | empty-asm statement (§F5 unchanged), or C11-named `atomic_signal_fence` extern (libatomic exports it; tcc ✅) | never emit `__atomic_signal_fence` — tcc does not know it |

The `-latomic` driver decision (F2) becomes: **posix ⇒ always link `-latomic` for tcc/clang
driven jobs (gcc doesn't need it but it's harmless if present); never on Windows** — unless
the §13.5 defs approach is adopted, which eliminates libatomic entirely.

### 13.5 NEW RECOMMENDED FORM (supersedes the F1.5 matrix): generic calls + exact-signature pure-C preamble definitions

`src/builtin_defs_probe.c`: emit **generic `_n` call sites** AND pure-C **definitions** of
`__atomic_load_n/store_n/...` (exact signatures, memcpy/loop-based, using suffixed calls or
plain non-atomic fallbacks per op) in the preamble:

- gcc/clang: **accept the definitions** (warning appears for mismatched sig; exact sigs are
  silent) and **inline the builtins at call sites** — definitions are dead code, can be
  emitted `static` (unused-function warnings suppressible by being referenced or by
  `(void)`-self-ref trick; verify)
- tcc: has no builtins → resolves calls to **our definitions** — no libatomic anywhere

Result: **one C artifact, zero external dependencies, all platforms** — the strongest
possible fulfillment of the compiler-independence rule (§1.4.4). Remaining verification
before adopting in F1 (add to P1 gate):
1. gcc `-Wall -Wextra` silence with exact signatures + referenced defs (probe showed
   mismatch warning when sigs differ; exact sigs expected silent — confirm per op)
2. full op × width matrix through this form (probe only covered load/store)
3. confirm `static` defs are fully discarded by gcc/clang (no code-size regression), else
   emit them non-static behind comptime `def.tcc` guard — allowed, since it is a
   **link-symbol availability** decision, not a codegen spelling change (the call sites are
   identical either way; §1.4.4's "link flags never change emitted C" is about user
   compiler choice — def.tcc-driven preamble additions are compiler-owned emission, still
   one artifact per target).
4. **Windows amendment (§14.2)**: promoted to **primary strategy on all platforms** —
   Windows has no libatomic (§14.1), so the §13.4 `-latomic` route exists only as a posix
   fallback. The verification items above remain P1 gate items; add the §14.7
   Windows-specific verification list to the same gate. For the synthesized ops (RMW
   synthesis, fence synthesis), build on the preamble **definitions**, not suffixed
   libatomic symbols, so the artifact stays self-contained on Windows.

### 13.5.1 `__sync_*` family: TCC does not support it — confirmed

`src/misc_constructs.c`: `__sync_synchronize`, `__sync_bool_compare_and_swap`,
`__sync_fetch_and_add`, `__sync_lock_test_and_set` — all implicit-declaration + unresolved
under tcc. gcc/clang fine. **Never emit `__sync_*`.** (This also kills any idea of using
`__sync_synchronize` as a tcc-compatible full barrier; the §13.4 fence synthesis is the
tcc-compatible form.)

### 13.6 Universal constructs — final probe status

| Construct | gcc | clang | tcc 0.9.28rc | Verdict |
|---|---|---|---|---|
| generic `__atomic_*_n` builtins | ✅ inline | ✅ inline | ❌ unresolved | never emit |
| multi-op no-suffix `__atomic_*` | ✅ | ✅ | ⚠️ parses; pointer-typed args+returns; signature divergence | never emit |
| `__sync_*` family | ✅ | ✅ | ❌ unresolved | never emit |
| suffixed `__atomic_*_N` symbols | ✅ builtin/inline | ✅ via libatomic | ✅ via libatomic | **preferred** |
| exact per-size decls of suffixed names | ✅ silent | ✅ | ✅ | mandatory if emitting suffixed calls |
| statement exprs: value/pointer/nested/discarded | ✅ | ✅ | ✅ zero warnings | proven (F1.4 synthesis OK) |
| `volatile` declarations + ptr-to-volatile | ✅ | ✅ | ✅ | F4 form proven |
| empty-asm `__asm__ __volatile__("" ::: "memory")` | ✅ | ✅ | ✅ | F5 form proven |
| non-empty x86 asm (`pause`, `mfence`) | ✅ | ✅ | ✅ | x86-only; ARM asm still unprobed (E.6) |
| `atomic_signal_fence` C11 extern (libatomic) | n/a (builtin) | n/a | ✅ links+runs | posix alternate form |
| `__atomic_signal_fence` builtin | ✅ | ✅ | ❌ unresolved | never emit |
| defining `__atomic_load_n` etc. as pure C | ✅ accepted | ✅ accepted | n/a (it just calls them) | §13.5 form |
| `__atomic_fetch_{and,or,xor,nand}_N` in libatomic | ✅ | ✅ | ✅ | bitwise RMW covered, incl. nand |
| max/min in libatomic | — | — | — | **do not exist** — synthesize |

### 13.7 CAS ABI check (5-arg suspicion)

`src/cas_abi_check.c` — the 5-arg call site (exactly as `nix/atomic.h` ships: missing the
`weak` slot) ran without abort and without value corruption in 1000 iterations under gcc
-O0/-O2, clang -O0/-O2 and tcc on linux-x86_64. Interpretation: the ABI mis-binding is
**real by construction** (5 args cannot fill 6 slots; the failure order register is
uninitialized), but the failure mode on this target is "garbage failure order that happens
to be a small valid-looking integer" — silent mis-semantics, not a crash. The suspicion
stands as **confirmed-by-construction, crash-unreproducible on SysV x86-64**. The code is
deleted regardless.

### 13.8 Linkage reality check (posix)

- gcc's libatomic **dev symlink** lives only in gcc's private dir
  (`/usr/lib/gcc/x86_64-linux-gnu/15/libatomic.so`); the runtime is at
  `/usr/lib/x86_64-linux-gnu/libatomic.so.1`. Our driver's `-latomic` must not assume the
  dev symlink exists (tcc could not find `library 'atomic'` by name) — link the resolved
  `.so`/`.so.1` path or add the gcc lib dir to the search path. On Alpine/musl, verify the
  package provides the symlink.
- tcc-produced binaries link `libatomic.so.1` fine (ldd verified).
- libatomic exports (nm verified): suffixed family incl. nand, multi-op names,
  `atomic_thread_fence`, `atomic_signal_fence`, `atomic_flag_*`. **No** `__atomic_thread_fence`,
  **no** `__atomic_signal_fence`, **no** `__atomic_*_n` generic names, **no** max/min.
- **Windows (§14.1)**: there is **no libatomic** to link — MSYS2 ships `libatomic-1.dll`
  only inside `mingw-w64-x86_64-gcc-libs` (a runtime DLL for gcc-built artifacts, not an
  import-lib story for clang/tcc). Windows targets are therefore zero-external-dependency
  by construction under the §13.5 defs form (§14.2); any `-latomic` on a Windows target is
  a bug.

### 13.9 Remaining probes

**Windows probes — now locally runnable (this is a Windows machine; results → §13.10):**

1. Bundled **Windows tcc** (`lib/tcc/tcc.exe`, native x86_64, msvcrt): suffixed forms
   parse/link behavior; §13.5 defs form compile+run; empty-asm + non-empty x86 asm;
   GNU statement expressions `(*({…}))` (linux-tcc proof may not transfer — different
   host ABI). Details in §13.10.
2. **Baseline health**: does the current `cruntime/` path link under Windows tcc at all?
   (§14.4 — tcc resolves `__atomic_*` names as plain externs; no libatomic on Windows.)

**Cross/other-target probes (unchanged from §12 E):**

3. TCC on **macos**: does it accept the builtins (macOS ships no libatomic ⇒ §13.5 defs or
   F7)?
4. TCC **non-empty inline asm on ARM/ARM64** (E.6) — decides hardware fences on the
   Windows-ARM64 corner.
5. gcc/clang `-Wall -Wextra` silence + dead-code elimination for the §13.5 defs, full
   op × width matrix (P1 gate item).
6. **NEW**: confirm the §13.5 defs approach under `-O2` code-size inspection (defs fully
   discarded by gcc/clang).

**Bottom line for F1**: emit suffixed `__atomic_*_N` calls with exact per-size preamble
declarations (or, preferably, the §13.5 generic+defs form); synthesize fences via dummy-cell
exchange; synthesize max/min via statement expressions; never emit `_n`/no-suffix/`__sync_*`
forms; `-latomic` on posix for tcc/clang jobs, never on Windows.

> **Windows amendment (§14.2)**: the "preferably" is now the plan — §13.5 generic+defs is
> the primary form on **all** platforms, which makes the last sentence automatic (nothing
> links libatomic anywhere; on Windows there is nothing to link). §14.2 pins the exact
> recipe; §14.7 the verification owed; §13.10 the Windows probe battery that fills the
> remaining unknowns.

### 13.10 Windows probe battery (P0-mandatory — this is a Windows machine)

Motivation: the §13 linux-host results do not automatically transfer to Windows — the
bundled tcc here is a **native Windows build** (`lib/tcc/tcc.exe`, `libtcc.dll`,
`libtcc1.a`, version 0.9.28rc 2026-08-09 mob@2ba12e8, x86_64 Windows, msvcrt CRT), i.e. a
different host ABI, different CRT, and a target (Windows) with **no libatomic at all**
(§14.1). Probes live in `lang/compiled/atomic_probe_win/src/` (gitignored area); results
are recorded in the table below as they complete.

| # | Probe | Expected | Result |
|---|---|---|---|
| P1 | Suffixed `__atomic_*_N` calls under Windows tcc — parse? link? (expect: parse OK, unresolved at link — no libatomic) | parse ✅ / link ❌ | ☐ TODO |
| P2 | **§13.5 defs form** under Windows tcc — generic `_n` call sites + pure-C defs compile, link, run | ✅ | ☐ TODO |
| P3 | §13.5 defs form under Windows gcc (MinGW-Builds gcc 13.2.0 on PATH) — exact-sig defs accepted, builtins inlined, `-Wall -Wextra` silent | ✅ | ☐ TODO |
| P4 | §13.5 defs form under clang for Windows (if installed — ☐ check availability) | ✅ | ☐ TODO |
| P5 | Empty-asm statement `__asm__ __volatile__("" ::: "memory")` under Windows tcc | ✅ (`win/atomic.h` evidence) | ☐ TODO |
| P6 | Non-empty x86 asm (`pause`, `mfence`) under Windows tcc | ✅ (`win/atomic.h` uses `pause`) | ☐ TODO |
| P7 | GNU statement expressions `(*({…}))` under Windows tcc — value/pointer/nested/discarded (linux-tcc §13.6 proof may not transfer; different host ABI + struct-return conventions) | ✅ | ☐ TODO |
| P8 | `volatile` declarations + ptr-to-volatile under Windows tcc | ✅ | ☐ TODO |
| P9 | **Baseline health** (§14.4): compile the current library's `cruntime/atomic.c` under Windows tcc — does the shipped code link at all today? | ❌ expected (silent breakage) | ☐ TODO |
| P10 | LLP64 typing check: exact-sig defs spelled with `unsigned long` under Win64 gcc — confirm the §13.3 mismatch failure mode, then confirm the fixed-width-typedef spelling is silent | mismatch ❌ / typedefs ✅ | ☐ TODO |
| P11 | Golden-C harness through the full 2c pipeline on this machine (`TCCCompiler --emit-c` → compile snapshot with gcc + tcc at `-O0`/`-O2` → run) | ✅ | ☐ TODO |

Environment note (recorded 2026-09-09): `gcc` on PATH = `x86_64-posix-seh-rev0, Built by
MinGW-Builds project 13.2.0` (msvcrt-based, `C:/ProgramData/mingw64`); no clang found on
PATH; no MSYS2 mingw64/ucrt64 installs detected at the standard locations. P4 updates the
doc if clang appears or is installed.

**Gate**: P2, P3 and P9 must complete before starting P1/F1 implementation on Windows;
P5–P8 must complete before F7 (fallback layer) work.

---

## 14. Appendix — Windows amendments (2026-09-09)

This appendix amends the plan for Windows as a **first-class development and target
platform** (development now happens on Windows). Each amendment states what changes and
why; where an amendment conflicts with earlier text, **this appendix wins**.

### 14.1 There is no usable libatomic on Windows — §13.5 is mandatory there

- MSYS2 ships `libatomic-1.dll`, but only inside `mingw-w64-x86_64-gcc-libs` — a runtime
  DLL accompanying gcc-built artifacts. There is no static import-lib story worth planning
  around, and a DLL dependency would violate the self-contained-output philosophy (§1.4)
  anyway.
- Therefore: `def.windows` targets **never** link `-latomic`, never reference suffixed
  libatomic symbols, and get the §13.5 defs form (or F7 fallback behind it). Any libatomic
  reference in a Windows-target artifact is a bug (§12 D.9).
- Consequence for F2: the posix `-latomic` branch becomes a posix-only fallback; the
  Windows branch links nothing (see §14.3).
- Consequence for §6/§F1.5 matrices: all Windows rows use §13.5 defs form; fences are
  always synthesized (§14.7); the F1.4 synthesized ops build on preamble **defs**, not
  suffixed libatomic symbols.

### 14.2 Primary emission recipe (amends §F1.4 / §F1.5 / §13.4–§13.5)

For every target (posix and Windows alike), the C backend emits:

1. **Call sites**: generic `_n` spellings — `__atomic_load_n / __atomic_store_n /
   __atomic_exchange_n / __atomic_compare_exchange_n / __atomic_fetch_{add,sub,and,or,xor,nand}_n`
   — plus the §F1.4 statement-expression synthesis for max/min/umax/umin and the
   dummy-cell exchange for fences.
2. **Preamble definitions** (§13.5): pure-C, exact-signature definitions of those names,
   memcpy/loop-based, built on plain (non-atomic) code + the universal primitives already
   proven (empty-asm, statement expressions, `volatile`). gcc/clang inline their builtins
   at the call sites and discard the defs; tcc resolves the calls to the defs.
3. **Typing rule (LLP64)**: every def/signature spells 8-byte types with the
   self-provided fixed-width typedefs (`uint64_t` emitted by `prepare_translate()` from
   `is64Bit`/`win64`) — **never** `long`/`unsigned long`/`long long` spellings. Windows is
   LLP64: `long` is 4 bytes on win64. The §13.3 per-size typing observations are LP64
   probe data pending Windows re-verification (§13.10 P3/P10).
4. **Result**: one C artifact per target, zero external dependencies on **all** platforms;
   `-latomic` disappears from the plan except as a posix fallback to §13.5 (§14.3).
5. **Sync scope**: unchanged from §F1.1 — only `System` accepted.

### 14.3 F2 linking policy (amends F2 acceptance criteria)

- `def.posix` targets: prefer §13.5 defs (zero deps); the `-latomic` fallback remains
  available for tcc/clang-driven posix jobs per §13.4, with the §13.8 symlink caveat.
- `def.windows` targets: **link nothing, ever**. F2's acceptance criterion becomes
  "Windows builds contain zero external atomic references (verified by inspecting the
  object/imports for `__atomic_*` / `libatomic`)".
- The `libatomic-1.dll` presence inside MSYS2 gcc-libs must NOT tempt a link flag — the
  artifact must not depend on it (users may not have MSYS2 at runtime).

### 14.4 Baseline health probe — the current library may be broken on Windows

Evidence chain: §13.1 (tcc resolves `__atomic_*` names as plain undefined externs) →
`win/atomic.h:39-47` calls `__atomic_compare_exchange` (multi-op family, pointer-arg
form) → tcc treats that as an undefined symbol too → and Windows has no libatomic
(§14.1). Therefore the current `cruntime/` path plausibly **fails to link under Windows
tcc today** (unless tcc-mob has Windows-specific builtin handling nobody knows about).
Probe §13.10 P9 settles this.

If confirmed: amend §1 to state that **Windows is broken today** — this plan is then a
correctness fix for Windows, not merely a portability cleanup. That also raises the
discovery priority of P9 to "before any other Windows work".

### 14.5 F7 fallback: fix the 64-bit CAS sketch for ARM64 (amends §5)

The §5 sketch `atomic_cas64_fallback(lo_ptr, hi_ptr, …)` as written is **not atomic**: two
loose "32-bit CAS steps" leave windows where the low half is swapped while the high half
still matches a stale compare (or vice versa), and any reader observing the cell
mid-sequence sees a torn value. Pin the semantics:

- The u64 cell is naturally aligned (Chemical guarantees u64 alignment), so a single
  logical CAS is achievable with the standard **double-CAS loop** pattern (as FFmpeg's
  `ManualInterlockedCompareExchange64` lineage does):
  1. read `lo`, `hi` of the target;
  2. CAS `lo` against `(expected_lo → desired_lo)`;
  3. re-read and verify `hi` still equals `expected_hi` — if not, retry the whole loop;
  4. CAS `hi` against `(expected_hi → desired_hi)`;
  5. re-read and verify `lo` still equals `desired_lo` — if not, **roll back** the first
     CAS (swap the old lo back) and retry;
  6. success: the cell now holds `desired` (for the duration between the two steps the
     cell may transiently hold a mixed value — acceptable **only** for 64-bit RMW/CAS
     emulation where all parties use the same protocol; single-writer/multi-reader torn
     reads remain a documented limitation, exactly as in the deleted header).
- A **signal fence (F5)** between the read and each CAS step inside the loop is mandatory
  (§F7 `wordtear.ch` note applies to 64-bit emulation too).
- lo/hi derivation is endianness-dependent — comptime-select via `def.little_endian` /
  `def.big_endian` (both exist on `TargetData`), as §5 already notes.
- All parties to a given cell must use the same protocol; document this in the library
  (mixed native-CAS / fallback access to the same cell is forbidden — enforceable only by
  convention since both live in one library).

### 14.6 No OS-atomic externs on Windows — document the temptation (amends F7)

Local evidence: `lib/tcc/lib/kernel32.def:437-441` exports **only the 32-bit family**
(`InterlockedCompareExchange`, `InterlockedDecrement`, `InterlockedExchange`,
`InterlockedExchangeAdd`, `InterlockedIncrement`) — no `_64` variants, no
`MemoryBarrier`. Additionally, on 32-bit Windows the `Interlocked*` functions are not
kernel32 exports at all (they are winbase.h intrinsics). A "just call Interlocked* from
the fallback" escape hatch is therefore a portability minefield on both sides.

**Rule**: F7 is pure-Chemical → pure-C only. No `Interlocked*`, no `MemoryBarrier`, no
`_Interlocked*` externs anywhere in emitted C (§12 D.9 adds this to the per-PR checks).
Everything is built from: plain loads/stores, `volatile` (F4), empty-asm signal fences
(F5), statement-expression CAS loops (F1.4 synthesis pattern), and the §13.5 preamble
defs.

### 14.7 Windows fence synthesis + verification owed (amends §6 fence column)

- `__atomic_thread_fence` is unresolved under tcc (§13.1) and Windows has no libatomic to
  provide the C11-named `atomic_thread_fence` → the **dummy-cell exchange synthesis is
  the only thread-fence form on Windows** (x86/x64 included, not just ARM64). Under the
  §13.5 recipe the synthesis calls the preamble **def** of the exchange primitive — fully
  self-contained.
- On Windows x86/x64 a full `seq_cst` fence degenerates to the compiler barrier + an
  atomic exchange (which tcc lowers to our def — see below). Honest capability statement
  (matching §F7's honesty rule): on **Windows + tcc**, hardware `mfence`/`lock`-prefix
  ordering comes only if tcc inlines something equivalent — it does not — so seq_cst
  fences are **compiler-barrier-strength** on that corner, exactly as the deleted header's
  `MemoryBarrier` shim was. gcc/clang on Windows inline real exchanges and get real
  ordering. This asymmetry is inherent to "any C compiler" on this target and must be
  documented in the library, not hidden.
- Verification owed before trusting this (add to the P1 gate):
  1. §13.10 P2/P3 — the defs form actually compiles and runs on Windows tcc + gcc.
  2. A fence litmus test (release-store + signal-fence + acquire-load) under Windows gcc
     `-O2` — and an explicit documented degradation note for tcc.
  3. The §13.5 open items (warning silence, op×width matrix, `-O2` dead-code check)
     re-run with mingw gcc + Windows tcc.

### 14.8 Concrete Windows harness definition (amends P0.4 / §8.2 / §12 D.2–D.3)

- "Every available host compiler" on Windows means: **MSYS2 mingw-w64 gcc** (record the
  CRT — ucrt vs msvcrt — and pin it in CI), **clang for Windows** (when installed), and
  the **bundled `lib/tcc/tcc.exe`**. All runnable from the repo's Git Bash environment.
- Snapshot scripts stay bash (repo convention).
- Warning-clean gate (§12 D.3) is defined **per platform**: on mingw additionally expect
  and baseline `-Wattributes`, dllimport/decorated-name noise; the gate is "no NEW
  warnings from atomic emission", not absolute silence, with the baseline recorded per
  compiler version.
- CI note (§7 P5): the Windows smoke job = MSYS2 setup + the golden-C harness from this
  section + the sequential test matrix from §8.1.

### 14.9 Amendment index (what changed, where)

| Section | Change |
|---|---|
| Header/Status | §13.5 promoted to primary strategy; Windows zero-deps stated |
| §1.4 | Windows self-sufficiency note (no libatomic → defs form or F7) |
| §F1.4 | Amendment callout: synthesized ops build on preamble defs |
| §F1.5 | Matrix Windows rows rewritten; linux-note caveat added |
| F2 | Posix-only `-latomic` fallback; Windows links nothing; acceptance criteria amended |
| §5 | (pointer to §14.5) — 64-bit fallback CAS semantics pinned in §14.5 |
| §6 | Windows rows: §13.5 defs form, synthesized fences, zero deps; cygwin added to non-goals |
| §8.2 | Windows harness pointer to §14.8 |
| §9 | New risk R8: LLP64 typing trap |
| §10 | DoD: Windows golden-C, LLP64 verification, zero-deps wording |
| §12 B | LLP64 `long` = 4 bytes warning |
| §12 C | Construct table: call-site-only `_n`, defs-form row, posix-fallback status, Windows fence note |
| §12 D | New checks 8/9/10 (LLP64, Windows zero-deps, Windows warning baseline) |
| §13.1 | Windows baseline-health follow-up pointer (§14.4, probe P9) |
| §13.3 | LLP64 warning block |
| §13.5 | Promotion to primary; verification items stay P1 gate; §14.7 list added |
| §13.8 | Windows: no libatomic bullet |
| §13.9 | Split into Windows (local) vs cross-target probes |
| §13.10 | NEW: Windows probe battery table + gates |
| §14 | NEW: this appendix |
