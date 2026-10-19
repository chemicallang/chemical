# Effect system & no-panic enforcement

**Chemical · Proposal 0005**

> A uniform, per-function mechanism to forbid effects — panics, unsafe operations,
> allocation, blocking — so a module or function can promise exactly what it will
> not do.

**Status:** Approved · **Area:** Language · effects · [review index](review.md)

## Goal fit

- **R2** enforceable no-panic
- **R1** unsafe auditing
- **R0 / TC** bitset checks, no compiler slowdown

## 1. Problem

Requirement 2 states that a module can forbid panics and the compiler errors if a
function can panic. Today nothing enforces this: `panic`, `abort` and (heavily
used) `unreachable` are unchecked. The same need keeps recurring for other
capabilities — a function that must not allocate, must not block, or must not use
`unsafe`. We want **one** mechanism that covers all of these, not a one-off
no-panic pass.

## 2. Decisions (from review)

1. **Syntax:** square brackets.
2. **v1 effects:** `panic`, `pure`, `blocking`, `unsafe`, `alloc`, `noreturn`.
3. **Per-function restrictions** are the core mechanism (a module default is
   allowed too) — any function may opt into forbidding any effect.
4. **No inference:** effects are declared, never inferred across current-module
   forward calls — emit an error instead of running a fixpoint pass.
5. **Traps are unsafe, not panic:** only explicit panics count as `panic`;
   `unreachable` and similar traps are `unsafe` (so `[no_panic, no_unsafe]` is what
   truly prevents an abort).

## 3. Concepts

- An **effect** is something an operation _does_: `panic`, `alloc`, `unsafe`,
  `blocking`, `noreturn`.
- A **restriction** is a function forbidding an effect in itself and everything it
  calls (transitively). `[pure]` forbids all effects.
- Every effect name has a positive form (this function _has_ it — used for
  declarations of foreign/forward functions) and a negative form `no_` (this
  function _forbids_ it). One uniform spelling for every case.

## 4. Syntax

```chemical
// Forbid panics and allocation: this function and everything it calls stay panic-free & alloc-free.
func parse(data : []u8) : Result<Ast, Error> [no_panic, no_alloc] { … }

// Forbid unsafe only.
func safe_only() [no_unsafe] { … }

// Shortest form: forbid all effects (a pure function — usable at comptime, optimizable).
[pure] func add(x : int, y : int) : int { return x + y }

// Declared effects (for a body not available locally, or an extern/forward function).
[panic, alloc] extern func libc_parse(x : *u8) : *Ast

// A property an operation/function has.
[noreturn] func fail(msg : string) { panic(msg) }
```

### Module policy

```chemical
# in chemical.mod — a default applied to every function in the module
no_panic = true
# per-function brackets override or extend the module default
```

## 5. Checking model (explicit-first, no fixpoint)

1. Each operation has a fixed effect (extending MIR's existing per-op `OF_Pure`).
2. A function's **actual effect set** = its body's operation effects ∪ the actual
   sets of the functions it calls.
3. **Dependencies** are already processed → their sets are read directly. A call
   to a function **earlier in the current module** is likewise known.
4. A call to a function **later in the current module** (or a cycle) is **not**
   resolved by analysis; the callee must carry an explicit declaration, otherwise
   the compiler **errors**. No fixpoint pass, no inference — the compile-time cost
   stays a bitset AND.
5. A function violates its restrictions if `actual_bits & forbidden_bits != 0`; a
   caller forbidding effect `e` cannot call a callee whose set has `e`.
6. **Cost:** one small bitset per function, one AND per call site, parallel per
   function, zero runtime cost.

## 6. Effects in v1

| Effect     | Restriction    | Use case                                                                                                     |
| ---------- | -------------- | ------------------------------------------------------------------------------------------------------------ |
| `panic`    | `[no_panic]`   | Explicit `panic()`, failing `unwrap`/checked operations that abort. Prevents aborting (R2).                    |
| `alloc`    | `[no_alloc]`   | No heap allocation — audio callbacks, hot paths, embedded/real-time.                                          |
| `unsafe`   | `[no_unsafe]`  | No `unsafe` blocks or raw trap operations (`unreachable`, raw OOB).                                           |
| `blocking` | `[no_blocking]`| No blocking calls — enforce non-blocking async coroutines.                                                    |
| `noreturn` | —              | Declares a function never returns (property, useful for divergence).                                           |
| `pure`     | `[pure]`       | Forbids all effects; required for `comptime`, enables CSE/parallelization.                                      |

## 7. Phased milestones

1. Effect bits + per-op effect table; storage on the function.
2. Parser: square-bracket specifiers (positive, `no_`, combined, `pure`).
3. Local effect computation from the body; diagnostics.
4. Call-site checking against declarations/known sets; the forward-reference error.
5. Module default policy from `chemical.mod`.
6. Map `unreachable`/raw traps to `unsafe`; map explicit panics to `panic`.
7. Tests: negative tests per effect + **common core-language tests** on
   interpreter, TCC and LLVM; performance validation (no regression).

## 8. Acceptance criteria

- A `[no_panic]` function that calls a panicking function fails to compile with an
  actionable diagnostic; likewise for `no_alloc`/`no_unsafe`/`no_blocking`.
- A current-module forward call without a declaration errors (no fixpoint
  inference happens).
- `unreachable` is reported as `unsafe`, not `panic`; thus `[no_panic, no_unsafe]`
  rejects `else unreachable`.
- No measurable compile-time or memory regression (bitset checks).
- Tests run on interpreter, TCC and LLVM.

## 9. Risks & open questions

- Mapping every existing operation to an effect (esp. containers' hidden
  allocations) is the bulk of the work and must be precise for `no_alloc` to mean
  anything.
- Effect polymorphism in generics: how a generic propagates its arguments' effects.
- How strictly `pure` interacts with the C ABI / `extern`.

## 10. Dependencies

Depends on: 04 (shares enforcement machinery; `unsafe` is the bridge) · Unblocks: a
real no-panic guarantee (R2), non-blocking async enforcement.

## 11. Evidence & references

- Use-case catalog: [0005-effect-system-analysis.md](0005-effect-system-analysis.md).
- `lang/docs/effect-system-proposal.md` (older syntax/scoring — reference only).
- Data: `unreachable` used 332× in `lang/libs`; `ThrowStatement` exists; MIR tags
  opcodes with `OF_Pure`.
- `lang/docs/proposals/DECISIONS.md` (decisions).
