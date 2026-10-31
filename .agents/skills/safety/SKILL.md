---
name: Safety Implementation
description: How the Chemical safety system is implemented and tested — the current enforced rules, stable SAFE-* diagnostics, auto-trait/Send/Sync behavior, borrow/move checking, async/FFI boundaries, known gaps, and the report→migrate→enforce workflow. Load before implementing or reviewing any safety rule, borrow-checker change, Send/Sync change, or unsafe-boundary check.
---

# Chemical Safety Implementation

This skill is the working guide for the safety program. The canonical long-form
plan is [`lang/docs/safety-implementation-plan.md`](../../../lang/docs/safety-implementation-plan.md).
The source tree wins over any status note here.

## Current status (2026-10-31)

The enforceable core is implemented and green. “Complete” below means **complete
for the safe API surface we currently expose**, not full language parity.

| Area | Status |
|---|---|
| Diagnostics/audit suites | Implemented: coded errors, negative/safety suites, whole-tree audit gate |
| Init / moves / drops | Implemented in MIR (`MIRMoveCheck`); partial-array drop elaboration remains |
| Borrows / lifetimes | Implemented for the enforceable surface (regions, NLL loans, storage lifetimes, return regions, variance, temporary extension, provenance, HRTB call regions) |
| `Send` / `Sync` | Auto traits + explicit/negative impls + nested conditional generic impls; data-race primitives |
| Unsafe / FFI | Enforced raw-pointer/reference/asm/extern boundaries; C ABI by-value support is x86-64 SysV only on LLVM |
| Validity / runtime | Bounds, overflow/div/shift, float→int, allocation/layout, packed/transparent, abort-only panic |
| Async | Shipped, `Pin`/Waker/frame loans; `FutureHandle<'a, T>` frame lifetime, `SAFE-LIFE-005` retaining-API escapes, and `spawn_send` `'static` check |
| Threads | Safe `spawn`/`spawn_closure`/scoped threads, `Send`-checked captures, TSAN smoke |
| Type-system / stdlib parity | M1/M7 remain; M2/M8 remain |

Latest measured baseline:

| Gate | Result |
|---|---|
| TCC main | `2366/2366` |
| LLVM main | `2364/2364` |
| `@test` runner | `54/54` |
| TCC/LLVM negative | `664/664` |
| TCC/LLVM safety | `64/64` |
| TCC/LLVM async | `58/58` |
| TCC libs | `730/730` |
| `MIRTests` | `4643 checks` |
| TSAN smoke | pass (`--llvm --thread-tsan`) |
| Whole-tree audit | `unsafe_blocks 3768`, `unsafe_values 4898`, `raw_pointer_derefs 1065`, `panic_calls 104` |
| Audit result | `no growth` against `lang/docs/safety/audit-baseline.json` |

The audit totals increased from the historical `3661/4890` in the 2026-10-31
audit because `SafetyReportVisitor` previously skipped **every**
`ImplDefinition` body. It now walks impl bodies, so the larger number is the
first honest whole-tree count, not a new unsafe-code regression.

## What is enforced (by domain)

### Unsafe boundaries

- `SAFE-UNSAFE-001` — raw-pointer dereference/projection/index/arithmetic/
  delete/dealloc in safe context. Dereferencing a reference is safe.
- `SAFE-UNSAFE-002` — call to `unsafe func`/`@extern`/unsafe function pointer,
  unsafe-to-safe function-pointer coercion, and unsafe generic calls.
- `SAFE-UNSAFE-003` — inline assembly outside `unsafe`.
- `SAFE-PROV-001` — raw pointer/integer/null converted to a reference.
- `SAFE-FFI-001` — `@extern` call from safe context.
- `SAFE-FFI-002` — `@extern` function passing/returning a Chemical variant by
  value (no C layout).
- `SAFE-FFI-003` — LLVM by-value `@extern` aggregate on a target other than
  x86-64 System V; use the C backend or a supported target until `MIRABI` is
  target-aware.
- `SAFE-SCOPED-001` — `@scoped_callback` parameter used for anything other than
  forwarding to a join-before-return scoped API (or another audited
  `@scoped_callback` function).
- `SAFE-GLOBAL-001` — mutable top-level `var`, `static mut`, or `@extern`
  global access outside `unsafe`; thread-local/comptime globals exempt.
- `SAFE-GLOBAL-002` — immutable global whose type is not `Sync` (including
  explicit/conditional `!Sync` and nested generic fields).
- `SAFE-TRAIT-001` — implementing an `unsafe interface` without `unsafe impl`.
- `SAFE-VIS-001` — private/internal member initialized or accessed outside its
  type/module (covers forged handles such as `Sender`, `Mutex`, `Arc`, …).
- `SAFE-VALID-006` — `@volatile` access; volatile is not atomic and concurrent
  access is a data race.
- `SAFE-VALID-007` — union field read (named or public anonymous) without
  `unsafe`; writes are allowed.
- `SAFE-VALID-008` — reference to a field of a packed aggregate (direct,
  nested, or array element).

### Init / moves / drops

MIR is the sole producer of init/move/drop state:

- `SAFE-INIT-001` — read/move/drop of an uninitialized place.
- `SAFE-MOVE-001` — move of an already-moved value, access/assignment after a
  move, or moving a destructor-bearing array element out by index.
- `SAFE-MOVE-002` — moving a non-`Copy` by-value callback capture out of a
  reusable callback (return/var init/assignment/by-value argument).
- `SAFE-DROP-001` — invalid `@delete` signature (value self, missing self,
  non-void return, async, etc.).
- Partial struct/array initialization and per-path drop flags are modeled.
  Field-wise drop elaboration for partial **array** moves remains pending.

### Borrows / lifetimes

`compiler/mir/MIRBorrowCheck.cpp` owns the enforced rules:

- `SAFE-BORROW-001` — overlapping live loans with at least one mutable loan.
- `SAFE-BORROW-002` — move/drop of a place while a borrow of it is live.
- `SAFE-BORROW-099` — a borrow/lifetime pattern the checker cannot prove safe
  (e.g. deeper aggregate HRTB nesting, mixed local/parameter multi-borrow
  referents, distinct non-local regions). This is an explicit rejection, never
  a guessed-safe acceptance.
- `SAFE-LIFE-001` — returning a reference to a local/temporary/frame field.
- `SAFE-LIFE-002` — a loan still live when the referent's storage ends.
- `SAFE-LIFE-003` — returned reference does not outlive the declared return
  region (add a `where 'a: 'b` bound when applicable).
- `SAFE-LIFE-004` — a type does not satisfy a `'static` bound/lifetime argument
  (stack reference submitted to a thread, stored in a `'static` container,
  etc.).
- `SAFE-LIFE-005` — a value containing a `FutureHandle` is passed to a safe
  callee that is not a known future consumer and does not return a
  future-containing result; the callee could retain the handle past its
  referent.

Region signatures are built from declarations before body lowering; MIR types
are lifetime-erased and regions live in per-function side tables. An async
function returning a `FutureHandle` gets a synthetic elided return region and
outlives edges from every borrowed parameter; `MIRBorrowCheck` also treats a
`FutureHandle` as a borrow view so the input loan follows the handle through
locals, fields, and ordinary by-value wrappers. Temporary
lifetime extension, variance, `PhantomData`, `Pin`, Waker ownership, and
async frame loans are implemented for the supported surface.

### Copy / validity / zeroing

- `SAFE-COPY-001` — a user type used under a `Copy` bound without an explicit
  `impl Copy` (primitives, pointers, references, functions, and arrays of Copy
  are built-in).
- `SAFE-COPY-002` — `impl Copy` on a destructor-bearing type.
- `SAFE-COPY-003` — `impl Copy` on a type with a non-Copy field.
- `SAFE-ZERO-001` — safe `zeroed<T>()`/`= zeroed` without `T: Zeroable`.
- `SAFE-VALID-001` — `zeroed<T>()` would create a null reference.
- `SAFE-VALID-002` — non-bool to `bool` cast in safe code.
- `SAFE-VALID-003` — integer literal that is not a declared enum member.
- `SAFE-VALID-004` — `zeroed<Enum>()` when no member has tag 0.
- `SAFE-REPR-001` — invalid `@repr("transparent")` shape.
- `SAFE-BOUNDS-001` — constant index outside a fixed-size array.
- Dynamic array bounds, vector/deque/string/string_view/span/map checks panic at
  runtime. Integer `/`/`%` and shifts are checked; `--overflow-checks` adds
  checked signed `+`/`-`/`*`/negation. Float→int is defined (NaN→0, saturation).

### Send / Sync / threads

- `SAFE-AUTO-001` — `Send`/`Sync` implemented without `unsafe impl`.
- `SAFE-AUTO-002` — invalid/conflicting negative or generic auto-trait impl.
- `SAFE-SEND-001` — a future frame submitted to `async::spawn_send` is not
  provably `Send + 'static` (direct async calls only).
- `SAFE-SEND-002` — a callback stored in `std::send_function`/`std::scoped_send_function`
  does not have a provable capture witness (`Rc`, borrowed, or erased).

Safe submission surfaces:

- `std::thread.spawn` / `spawn_void` — non-capturing entry, `T: Send + 'static`,
  `U: Send + 'static`; `JoinHandle<T>: Send + Sync` for `T: Send`.
- `std::thread.spawn_closure` — typed capture `C: Send + 'static`.
- `std::thread.scoped_spawn` / `_mut` and scoped closure entries — join before
  returning; shared borrow requires `T: Sync`, mutable borrow requires
  `T: Send`.
- `std::send_function<T>` — non-generic owned-capture parameter; marker
  `SendStaticCapture` is `Send` but deliberately `!Sync` (only `Send` is
  proven).
- `std::scoped_send_function<T>` — non-generic borrowed-capture parameter;
  marker `ScopedSendCapture` is `!Send`/`!Sync` so it cannot reach an unscoped
  API. `@scoped_callback` is compiler-checked: its parameter may only be
  forwarded to a known join-before-return scoped API (or another annotated
  function); returning/storing/capturing it is `SAFE-SCOPED-001`.
- `std::concurrent.submit_closure` — safe typed-capture pool submission.
- `async::spawn` / `spawn_local` / `spawn_blocking` / `concurrent.spawn` are
  explicit `unsafe` boundaries; the safe async path is `spawn_send` /
  `SendFutureHandle<T>`.

`Arc`/`ArcWeak` counts are protected by one `ArcInner` mutex; `upgrade` is a
test-and-increment under that lock, and the value destructor runs outside it
behind a `value_state` transition. `ArcWeak<T>` is `Send + Sync` when
`T: Send + Sync`. `recv_or` futures own a receiver
reference count and release it in `recv_drop`, so a future can outlive the
`Channel` handles. `MutexGuard`, `RwLockReadGuard`, and `RwLockWriteGuard` have
explicit `!Send`/`!Sync` impls. `UnsafeCell`/`Cell`/`RefCell` are `!Sync`;
`Mutex<T>` is `Send + Sync` for `T: Send`; `RwLock<T>` is `Send` for
`T: Send` and `Sync` for `T: Send + Sync`.

## Auto-trait model (important)

`compiler/safety/AutoTraits.h` derives `Send`/`Sync` structurally:

- raw pointers are neither; `&T` is `Send` iff `T: Sync`; `&mut T` is `Send`
  iff `T: Send`; references are `Sync` iff the referent is `Sync`;
- functions are `Send + Sync`; capturing functions use their capture
  environment;
- arrays/generic arguments/struct fields/variant payloads recurse;
- a bare generic parameter is not automatically `Send`/`Sync` without a bound;
- `dyn I` is conservatively **neither `Send` nor `Sync` nor `Unpin`** because
  Chemical does not yet represent `dyn I + Send + Sync` bounds and the erased
  implementor may be backed by `Rc`/raw pointers.

`TypeVerifierAutoTraitContext` (in `compiler/typeverify/TypeVerify.cpp`) is the
hook that evaluates **conditional generic positive and negative** impls at a
concrete instantiation. This is essential for nested fields:

```chemical
impl<T> !Send for Wrapper<T> where T: Copy { }
struct Outer { var w : Wrapper<int> }   // Outer must be !Send
```

The context also makes `Outer` honor `unsafe impl<T> Send for Wrapper<T> where
T: Send`. Without it, nested fields used only the raw field shape and could
accept a declared-`!Send`/`!Sync` member (unsound) or reject a valid positive
impl (false negative).

`Unpin` is a safe auto trait. Nested conditional `!Unpin` is treated
conservatively as negative because the `Unpin` path has no TypeVerifier context;
conditional positive `Unpin` impls are not evaluated (false rejection, not
unsoundness). `PhantomPinned` uses `impl !Unpin`.

The `GenericImplDecl` instantiation path also treats any registered generic
negative auto-trait impl as negative without evaluating its bounds. That can
reject a use where the bound does not apply (false negative), but never accepts a
declared-negative type.

## Known remaining gaps / blockers

These are verified gaps, not placeholders. Read this list before claiming full
memory safety.

1. **`FutureHandle<'a, T>` carries a frame lifetime; full inference remains.**
   Compiler-generated async returns use `'static` for an owning frame and an
   elided lifetime when a parameter/frame value borrows. A borrowed handle
   cannot satisfy a `FutureHandle<'static, T>` slot, `spawn_send` requires
   `'static`, and a borrowed future returned through the legacy
   lifetime-less `FutureHandle<T>` spelling is rejected. MIR tracks direct
   async-call loans and `SAFE-LIFE-005` rejects a future (or a
   struct/array/`Option` containing one) passed to a safe callee that is
   neither a known consumer (`block_on`/`spawn` in the actual async/std library
   namespace) nor returning a future-containing result. This closes the direct
   return, local scope, wrapper, method, struct-field, `Option`, `string_view`,
   and safe stash cases in the negative suite. Remaining: full call-site
   lifetime inference/unification, custom safe consumer wrappers
   (conservatively rejected), and borrowed futures hidden behind a reference
   or callback capture.
2. **Full target-aware C ABI remains M2.** LLVM now rejects by-value `@extern`
   aggregates on targets other than x86-64 SysV (`SAFE-FFI-003`); the C backend
   delegates to the host compiler. Windows/ARM ABI support, `repr`/layout
   coverage, variadics, callbacks, and FFI wrapper auditing remain.
3. **`@scoped_callback` is body-checked, with a fixed sink list.**
   `SAFE-SCOPED-001` rejects returning, storing, capturing, or passing the
   scoped parameter anywhere except a known join-before-return API (resolved by exact library
   namespace) or another annotated function. New scoped sink APIs must be added to that list; an
   arbitrary user-defined sink is intentionally not trusted.
4. **No `FnOnce`/`FnMut`/`Fn` callable model.**
   Non-`Copy` nested aggregate captures cannot be moved out of a reusable
   callback; they are an explicit lowering error. `SAFE-MOVE-002` prevents the
   old bitwise-copy/double-own bug. Implementing once-call semantics is
   M1.16/M6.6 work.

5. **HRTB precision is intentionally incomplete.**
   Deeper nested aggregate HRTB and multi-borrow callbacks mixing local and
   parameter/global referents (or distinct non-local regions) are explicit
   `SAFE-BORROW-099` rejections. This is a false-negative/rewrite requirement,
   not a guessed-safe acceptance.

6. **`dyn` has no auto-trait bounds.**
   Conservative `!Send`/`!Sync`/`!Unpin` avoids the alternative unsoundness
   (sending an `Rc`-backed `dyn` object), but `dyn I + Send` cannot be expressed
   and `Box<dyn I>`/`Arc<dyn I>` cannot cross threads. Full `dyn` bounds are
   M1.19.

7. **Async runtime allocation checks are current but must stay current.**
   Every future constructor in `yield.ch`, `time.ch`, `exec.ch`, `blocking.ch`,
   `combinators.ch`, `reactor.ch`, and `frame.ch` now checks its `malloc`; new
   constructors must use the same checked/`new` pattern. Broader stdlib
   allocation-failure coverage remains part of M7.
8. **The audit gate is now CI-wired, but platform breadth remains.**
   `.github/workflows/safety-audit.yml` runs `scripts/safety-audit.sh` on
   pull requests and `main`. The audit module list is host-platform-oriented;
   Windows/OS-gated sources need target-specific audit coverage, and
   `--update` remains a reviewed manual baseline change.
9. **Audit reporting details.**
   `SafetyReportVisitor` now walks `ImplDefinition` bodies (previously skipped
   entirely). Generic master/concrete impls can double-count some syntax, so the
   audit is conservative rather than a precise source-line count. The report
   also does not count `unsafe func` declarations or `unsafe impl` declarations
   as separate metrics; both deserve an explicit audit metric before release.

10. **Partial-array drop elaboration** remains pending; whole-array moves and
    field/element `Copy` paths are handled, but field-wise drops for partially
    moved fixed arrays are not.

11. **Interpreter boundaries.** MIR interpreter bounds/provenance/tombstone
    checks are implemented. Async/coroutine opcodes, inline asm, dynamic
    call/vtables, `Unreachable`/`Throw` are explicit hard interpreter errors
    where they have no comptime meaning; they are not silently executed.

12. **Panic unwinding is withdrawn.** Panic always aborts; there is no
    `catch_unwind`, `UnwindSafe`, or drop-on-unwind surface.

## Callback and `Fn` model

`std::function<T>` is a type-erased, destructor-owning callable. Captures are
always heap-indirect and alignment-aware, so moving the erased value is
memory-safe. It is not Rust's `FnOnce`/`FnMut`/`Fn` hierarchy: there is no
call-once/call-many distinction, no receiver mutability model, and a bare
`std::function` parameter erases the capture witness.

Use the witness-carrying forms when a signature must expose a guarantee:

- `std::send_function<T>` — verified `Send + 'static` capture; marker
  `SendStaticCapture` is `Send`, not `Sync`.
- `std::scoped_send_function<T>` — verified `Send` (borrow allowed) capture;
  marker is `!Send`/`!Sync`; only join-before-return scoped APIs accept it.
- `std::typed_function<F, C>` — expose the concrete capture environment `C`.

`@scoped_callback` is the compiler-checked contract for forwarding a
`scoped_send_function` parameter. Without it, constructing a borrowed capture
for a parameter requires `unsafe` (`SAFE-UNSAFE-002`). Use is additionally
checked by `SAFE-SCOPED-001`: returning, storing, capturing, or passing the
parameter to an unknown callee is rejected.

Capturing closures that borrow local storage require an `unsafe` boundary
unless they are passed to a recognized join-before-return scoped API. Owned
by-value captures stay safe. `SAFE-LIFE-001` rejects returning a callback that
borrows function-local storage; taint is followed through assignments, fields/
indexes, access chains, erased parameters, and variant/generic payloads.

## Non-negotiable rules

1. **No user opt-out.** No `SafeMode::Off`, module flag, CLI escape, or
   per-dependency exemption.
2. **No error-suppressing allowlist.** A rule is report-only until the in-repo
   tree is migrated, then enforced. Report-only is a development mode and must
   not ship as a silent release escape.
3. **`unsafe` is explicit.** It permits only inherently unchecked operations;
   it never disables ownership, initialization, or borrow checking.
4. **Never accept a pattern the checker cannot prove safe.** Emit a specific
   `SAFE-*` diagnostic and a rewrite suggestion.
5. **Every safety rule has a negative test, a positive test, and (where
   applicable) a runtime-failure test.**
6. **No diagnostic mentions the reference language.**
7. **Every commit is green.** Use the path→suite gate in the plan (§12.7).

## Report → migrate → enforce

Every new safety rule follows the same workflow:

1. **Report-only**
   - Add a stable `SAFE-*` code.
   - Emit a warning; behavior unchanged.
   - Add negative/positive tests and update the audit report.
2. **Migrate**
   - Fix every in-repo occurrence (libraries, tests, `build.lab`, CBI).
   - Report hits reach zero.
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

Coded output renders `error[SAFE-*]:` / `warn[SAFE-*]:`; uncoded output keeps
`error:`. Prefer asserting the code, not the full message, in negative tests.

## Diagnostic catalog (emitted today)

| Code | Meaning |
|---|---|
| `SAFE-AUTO-001` | `Send`/`Sync` implemented without `unsafe impl` |
| `SAFE-AUTO-002` | invalid/conflicting negative/generic auto-trait impl |
| `SAFE-BORROW-001` | overlapping live loans (at least one mutable) |
| `SAFE-BORROW-002` | move/drop of a place while a borrow is live |
| `SAFE-BORROW-099` | unsupported/unproven borrow/lifetime pattern |
| `SAFE-BOUNDS-001` | constant index outside a fixed-size array |
| `SAFE-COPY-001` | user type under a `Copy` bound without `impl Copy` |
| `SAFE-COPY-002` | `impl Copy` on a destructor-bearing type |
| `SAFE-COPY-003` | `impl Copy` with a non-Copy field |
| `SAFE-DROP-001` | invalid destructor signature |
| `SAFE-FFI-001` | `@extern` call in safe context |
| `SAFE-FFI-002` | by-value Chemical variant across `@extern` ABI |
| `SAFE-FFI-003` | LLVM by-value extern aggregate on unsupported target ABI |
| `SAFE-GLOBAL-001` | mutable/extern global access in safe context |
| `SAFE-GLOBAL-002` | immutable global whose type is not `Sync` |
| `SAFE-INIT-001` | use/move/drop of an uninitialized place |
| `SAFE-LIFE-001` | reference/local-storage escape |
| `SAFE-LIFE-002` | loan outlives referent storage |
| `SAFE-LIFE-003` | returned reference violates declared return lifetime |
| `SAFE-LIFE-004` | type does not satisfy `'static`/lifetime bound |
| `SAFE-LIFE-005` | future-containing value passed to a retaining safe API |
| `SAFE-MOVE-001` | move/access/assignment after move, or array-element move |
| `SAFE-MOVE-002` | non-Copy callback capture moved out of a reusable callback |
| `SAFE-PROV-001` | raw/integer/null converted to a reference |
| `SAFE-REPR-001` | invalid `@repr("transparent")` shape |
| `SAFE-SCOPED-001` | `@scoped_callback` parameter used outside a scoped sink |
| `SAFE-SEND-001` | async frame not provably `Send + 'static` |
| `SAFE-SEND-002` | callback capture not provably `Send`/`Send + 'static` |
| `SAFE-TRAIT-001` | `unsafe interface` implemented without `unsafe impl` |
| `SAFE-UNSAFE-001` | raw-pointer operation in safe context |
| `SAFE-UNSAFE-002` | unsafe/extern call or unsafe-to-safe coercion |
| `SAFE-UNSAFE-003` | inline assembly outside `unsafe` |
| `SAFE-VALID-001` | zeroing would create a null reference |
| `SAFE-VALID-002` | non-bool to `bool` cast in safe code |
| `SAFE-VALID-003` | integer literal not a declared enum member |
| `SAFE-VALID-004` | `zeroed<Enum>()` with no zero tag |
| `SAFE-VALID-006` | `@volatile` access outside `unsafe` |
| `SAFE-VALID-007` | union field read outside `unsafe` |
| `SAFE-VALID-008` | reference to a packed aggregate field |
| `SAFE-VIS-001` | private/internal member initialized/accessed outside its module |
| `SAFE-ZERO-001` | safe zero-initialization without `Zeroable` |

Retired/reserved codes (do not reuse):

- `SAFE-VALID-005` retired; runtime int-to-enum casts are checked in MIR and
  panic through `__chc_enum_fail`.
- `SAFE-DROP-002` retired; per-use dropck is `SAFE-LIFE-002`.
- `SAFE-OWN-001` and `SAFE-OVERFLOW-*` are reserved/never emitted as coded
  diagnostics today (move rules use `SAFE-MOVE-*`; overflow checks panic).
- `SAFE-ASYNC-*` and `SAFE-DEP-*` are reserved for future async/remote-dependency
  work; do not invent sub-codes ad hoc.

Do not reuse a code for a different rule. Do not renumber existing codes.

## Test harness

Compile-fail tests live in `lang/tests/negative/src/`:

```chemical
@test
func neg_example(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "... unsafe source ...";
    expect_compile_error(env, "example", ch, "SAFE-MOVE-001");
}
```

Helpers in `lang/tests/negative/src/main.ch`:

- `expect_compile_success(env, name, ch)`
- `expect_compile_error(env, name, ch, expected_substring)`
- `expect_compile_error_with_mod(env, name, ch, expected_substring, mod_content)`
- `expect_compile_warning_id(env, name, ch, expected_id)`
- `expect_compile_output_contains(...)`
- `expect_compile_output_not_contains(...)`
- `expect_emitted_symbol(...)`

Runtime/parity tests live in `lang/tests/safety/` and run with `--safety`.
Compile-fail tests stay in `--negative` so the default suites remain fast.
Async/channel tests live in `lang/tests/async/`; thread tests in
`lang/tests/src/stdlib/threadpool` and `lang/tests/libs/thread`.
TSAN smoke lives in `lang/tests/thread_tsan/`.

## Commands

```bash
# Fast compile-fail gate
./scripts/test.sh --tcc --negative --no-build

# Safety suite (smoke, report, runtime-panic, allocation overflow)
./scripts/test.sh --tcc --safety --no-build

# Async suite (includes channel auto-trait + runtime tests)
./scripts/test.sh --tcc --async --no-build
./scripts/test.sh --llvm --async --no-build

# Read-only audit
./cmake-build-debug/TCCCompiler <module> --no-cache --safety-report
./cmake-build-debug/TCCCompiler <module> --no-cache --safety-report json

# Lower + verify without emitting/linking
./cmake-build-debug/TCCCompiler <module> --check --no-cache

# M6 thread data-race smoke under TSAN (system LLVM resources)
./scripts/test.sh --llvm --thread-tsan
# equivalent standalone:
./scripts/thread-tsan.sh

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
| Auto traits / structural facts | `compiler/safety/AutoTraits.h`, `compiler/symres/ImplementationsIndex.*` |
| Audit report | `compiler/safety/SafetyReport.*`, `scripts/safety-audit.sh`, `lang/docs/safety/audit-baseline.json` |
| MIR verifier | `compiler/mir/MIRVerifier.*` |
| MIR move/init/drop | `compiler/mir/MIRMoveCheck.*` |
| MIR borrow/lifetimes | `compiler/mir/MIRBorrowCheck.*`, `MIRRegionSignature.*` |
| Runtime checks | MIR lowering (`compiler/mir/MIRLowerStmt.cpp`, `MIRLowerExpr.cpp`, `MIRLowerCall.cpp`, etc.), C/LLVM emitters |
| Interpreter guards | `compiler/mir/MIRInterpreter.*`, `MIRComptime.*` |
| Standard library | `lang/libs/core`, `lang/libs/std`, `lang/libs/cstd`, `lang/libs/async` |
| Tests | `lang/tests/negative/src/safety_*.ch`, `lang/tests/safety/`, `lang/tests/async/`, `lang/tests/thread_tsan/` |
| Plan | `lang/docs/safety-implementation-plan.md` |
| Audit metrics | `lang/docs/safety/migration-metrics.md` |

## Current architecture reminders

- Backends never read the AST; safety facts belong in MIR. The exception is the
  front-end declaration/type-check passes, which produce the facts.
- MIR types are lifetime-erased; regions live in per-function side tables.
- `MIRMoveCheck` is the sole producer of init/drop state; drop flags are
  elaborated from its plan.
- Every MIR-eligible function (including lambdas, async poll/drop, stubs) goes
  through `mir::verify_function`, `mir::check_function_borrows`, and
  `mir::check_function_moves` in `ASTProcessor`'s lowering validation.
- `--check` still lowers and validates; it is not an AST-only shortcut.
- Bounds lower as `Compare` + `CondBr` + panic call, not a mid-block opcode.
- C signed overflow/float→int are lowered explicitly for TCC and LLVM.
- `unsafe` never disables borrow/init checking; `unsafe` is a boundary marker.
- The safety audit runs after type checking and before codegen; it now walks
  impl bodies as well as module/type declarations.

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
- [ ] If touching auto traits: test nested concrete + conditional generic
      positive/negative impls, not just the top-level bound.
- [ ] If touching async: consider the `FutureHandle` borrow-escape gap and
      `spawn_send` frame check.
- [ ] If touching FFI: remember the LLVM C ABI is x86-64 SysV only.
