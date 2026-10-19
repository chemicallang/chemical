# Effect system — use-case analysis

**Chemical · Proposal 0005 · analysis**

> Requirement 2 names **no-panic**, but the previous design proposed
> `pure`/`unsafe`/`alloc`/`mut`/`suspends`. Before designing, catalog which
> effects actually earn their keep.

**Status:** Analysis — needs decision · **Area:** Language · effects ·
[review index](review.md)

> **Decided already**
>
> - **Syntax:** square brackets — `[effect] func …` / `func f() : T [effect]`.
> - **No expensive inference.** A dependency's body is already processed, so its
>   functions' effects are known and may be read. A call to a function in the
>   **current module** (possibly defined below) is **not** inferred — its effects
>   must be declared, else the compiler **errors**. No fixpoint passes.

## 1. What exists today (so we don't duplicate it)

- `Result`/`Option` is the primary error mechanism; `throw`/`try`/`catch` also
  exist (`ThrowStatement`).
- The `unsafe` marker plus the safety plan's `@unsafe` propagation (F2) already
  handle unsafe auditing.
- `unreachable` is used 332 times in libraries (e.g. `else unreachable`) and is an
  unchecked abort today.
- `comptime` evaluation exists and implicitly requires side-effect-free code.
- The async runtime bridges blocking work with `spawn_blocking` (a known pain
  point).
- MIR already tags opcodes with `OF_Pure` (purity is tracked per-op, not checkable
  per-function).

## 2. Candidate effect catalog

| Effect                  | Use case                                                                     | Value         | Cost                                      | Already covered?                              |
| ----------------------- | ---------------------------------------------------------------------------- | ------------- | ----------------------------------------- | --------------------------------------------- |
| `panic` / no-panic      | A module guarantees it never aborts; forces `Result`-based handling (R2).     | **High**      | Low — 1 byte/function, an AND at each call site | No — `unreachable`/`panic`/`abort` unchecked |
| `pure`                  | No observable side effects; required for `comptime`; enables CSE/parallelization. | **High**  | Low                                       | Partly — MIR `OF_Pure` is per-op, not checkable per function |
| `blocking` / no-blocking| Forbid blocking calls inside async coroutines (prevents stalling the reactor).| **High** (async) | Medium — needs async integration       | No                                            |
| `unsafe` / no-unsafe    | Audit that a module uses no `unsafe`.                                         | Medium        | Low — reuses the existing marker          | Partly — safety plan F2 targets it            |
| `alloc` / no-alloc      | Guarantee no heap allocation (audio callbacks, hot paths, embedded/real-time).| Medium        | High — hidden allocs in containers        | No                                            |
| `suspends`              | Track functions that may suspend.                                             | Low           | Low                                       | Partly — async typing implies it              |
| `mut` / readonly        | Aliasing / immutability guarantees.                                           | Low           | —                                         | Overlaps the borrow checker (04)              |
| `noreturn`              | Function never returns.                                                       | Low           | Low                                       | No                                            |

## 3. The checking model (explicit-first)

- Each function stores a small effect bitset (declared effects, plus the effects
  observed in its own body from resolved callees).
- **Dependencies** are already processed → a caller reads their bitsets directly
  (no inference pass).
- **Current-module forward calls** (definition may come later) cannot be resolved
  cheaply → the callee's effects must be **declared**; if missing, the compiler
  errors. No fixpoint, no whole-program analysis.
- **Module policy** in `chemical.mod` (e.g. `no_panic = true`) applies to every
  function in the module.
- Checking is a bitset AND at the call site — O(1), no runtime cost, parallel per
  function.

## 4. Recommendation for v1

Build the effect that Requirement 2 actually names — **`panic`** — plus **`pure`**
if `comptime`/optimization needs a checkable guarantee. Add **`blocking`** only if
the async runtime wants to enforce non-blocking coroutines. Defer `alloc`, `mut`
and `suspends` until a concrete use case demands them (or is already covered by
04).

## 5. What I need you to decide

- **Which effects are in v1?** (my recommendation: `panic` + `pure`).
- Does `no_panic` treat `unreachable`, index-out-of-bounds and traps as `panic`?
  (If not, a no-panic module could still abort.)
- Module policy granularity: one flag per module, or per-function override too?
- Effect polymorphism: how generics propagate the effects of their arguments.

## 6. References

- `lang/docs/effect-system-proposal.md` (older syntax/scoring — use for ideas, not
  as authority; it did not include `panic`).
- `lang/docs/proposals/DECISIONS.md` (syntax + inference decisions).
- `lang/docs/async-remaining-work.md` (blocking/async context).
