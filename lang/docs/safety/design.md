# Chemical safety — design

This is the short design entry point. The full pre-split design record (threat
model, every rule, borrow-checker algorithms, milestones, risk register) is kept
verbatim in [`history.md`](history.md) — sections 4–19 there are the detailed
design, and the local links below point at the same material.

Current status: [`current-status.md`](current-status.md).
Live evidence table: [`../safety-sharpen/STATUS.md`](../safety-sharpen/STATUS.md).

## 1. Safety model

Chemical adopts Rust-familiar **safety semantics** (ownership, move semantics,
borrow checking, `Send`/`Sync`) while keeping Chemical syntax. It is not a Rust
clone: features that cost compile time for little verification value are
rejected, and no new syntax is added without a concrete safe API that needs it.

The safety contract is:

1. Safe code cannot cause a memory error, data race, or forged invariant.
2. Operations the compiler cannot prove are explicit `unsafe`, and every unsafe
   boundary is auditable.
3. `panic` is abort-only (no unwinding, no `catch_unwind`); recoverable failure
   is `Result`.
4. A function/module can forbid effects (panic, allocation, unsafe, blocking)
   and the compiler rejects a body that can perform one.

## 2. Architecture

The AST is a front-end artifact. Every safety check that needs body facts runs
on **MIR**, the single shared mid-level IR that all backends consume:

| Layer | Responsibility |
|---|---|
| Parser (`parser/`) | Syntax, effect lists, removed-feature diagnostics |
| Symres (`compiler/symres/`) | Declarations, generics, lifetimes in types |
| Type verification (`compiler/typeverify/`) | Types, implicit conversions, unsafe/FFI boundaries, definite assignment |
| MIR lowering (`compiler/mir/MIRLowerer*`) | Places, moves/drops, effects, ABI, lifetimes carried into MIR |
| MIR checks (`MIRMoveCheck`, `MIRBorrowCheck`, effect check) | Move/borrow/init/effect enforcement |
| Backends (`MIREmitter`, `compiler/CodegenMir.cpp`) | C and LLVM emission from MIR only |
| Interpreter (`MIRInterpreter`) | Executes the same MIR for `--interpret`/comptime |

Because backends never see the AST, a safety fact a backend needs is published
by MIR, never re-derived from source form.

## 3. Borrowing and lifetimes

- Regions/loans are computed in MIR; NLL ends a loan at its last use.
- A call returning a reference carries the receiver loan, including when the
  reference is wrapped in an aggregate (`Option<&T>`, `Result<&T, E>`, a
  reference-holding struct). This closes the `vector::get` + `push`
  use-after-free.
- Views (`span<'a, T>`, `mut_span<'a, T>`, `str 'a`, `CStr<'a>`) carry their
  borrow lifetime; a `'static` escape is rejected.
- HRTB function-type parameters support named lifetime-generic bodies; anonymous
  closures that need the higher-ranked contract are a known gap (see
  `../safety-sharpen/P2.6-concurrency.md`).

## 4. Moves, drops, initialization

- Every local/place has a drop flag; moves clear the exact parent/element flag.
- Consuming `self` moves the receiver (Copy exempt) and rejects a move out of a
  pointer/reference projection (`SAFE-MOVE-004`).
- Assignment to a field through a pointer and to a global participates in move
  tracking.
- A variable declared without an initializer is uninitialized until a full
  assignment; reading it first is `SAFE-INIT-001` unless wrapped in `unsafe`.

## 5. Effects (proposal 0005)

The effect model is a per-function bitset plus an explicit declaration policy
(no inference/fixpoint). `[no_panic]` is enforced today on the lowered body:
direct calls must target a `[no_panic]` callee; indirect/dynamic calls, throws,
and generated runtime panics are rejected. Traps (`unreachable`) are an
`unsafe` effect, not a panic. Phase 3 adds `[no_alloc]`, `[no_unsafe]`,
`[no_blocking]`, `[pure]`, combined forms, and module defaults as opt-in
syntax.

## 6. Unsafe, FFI, and the safe API surface

- Safe-by-default names; raw-pointer/FFI forms live under `::raw`/`::ffi`.
- A public function whose parameter list takes a raw pointer is `unsafe func`
  (`scripts/safety-raw-surface-audit.py`).
- The C ABI is target-aware for the claimed targets; unknown targets and
  unauditable callback ABIs reject rather than guess.
- Legacy raw-buffer APIs are reviewed migration debt, enumerated by
  `scripts/safety-sharpen-api-audit.sh`.

## 7. Concurrency and async

- `Send`/`Sync` are auto traits with explicit/negative impls and conditional
  generic impls; guards are `!Send`; unbound `dyn` is conservatively `!Send` /
  `!Sync` (kept).
- Safe scoped APIs are join-before-return (`@scoped_callback` audited);
  `thread::scope` remains a spike until closure HRTB and capture-witness
  inference are expressible.
- Async frames carry lifetimes; `FutureHandle<'a, T>` distinguishes owning
  (`'static`) and borrowed frames.

## 8. Testing strategy

Layered gates: `--safety` (runtime/parity), `--negative` (compile-fail with
stable codes), `--removed` (removed-feature hints), `--libs`/`--api` (safe
surface), `--fuzz` (fixed malformed-input corpus), `MIRTests`,
`--llvm --thread-tsan`, ASAN, the whole-tree audit, and the public-API audit.
A safety rule is not done until its negative test is wired into CI.
