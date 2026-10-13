# Proposal decisions log

Running record of review decisions. Newest first. This is the source of truth for
*why* a proposal is in a given state.

## 2026-10-13 — third pass (effects closed)

| # | Proposal | Decision | Decided specifics |
|---|----------|----------|-------------------|
| 05 | No-panic effect system | **Approved** | **v1 effects:** `panic`, `pure`, `blocking`, `unsafe`, `alloc`, `noreturn`. **Core mechanism:** any function may declare **per-function restrictions** (`[no_panic]`, `[no_alloc]`, `[no_unsafe]`, `[no_blocking]`; `[pure]` = forbid all), with an optional module default. **Traps:** `unreachable`/raw traps are **`unsafe`**, not `panic` — so `[no_panic, no_unsafe]` is what truly prevents an abort. Effects are **declared, never inferred** across current-module forward calls (error instead). |

## 2026-10-13 — second pass (open items)

| # | Proposal | Decision | Decided specifics |
|---|----------|----------|-------------------|
| 05 | No-panic effect system | **Needs use-case analysis first** | **Syntax = square brackets** (`[effect] func …`) — confirmed. **No expensive inference:** a function whose body is already processed (a dependency) can have its effects read; a call to a function *in the current module* (possibly defined below) must be **declared**, otherwise **error** — do not invent passes to infer it. Still open: **which effects are actually needed** (use-case catalog produced). |
| 06 | Native GC mode | **Approved** | **Opt-in:** a module turns it on explicitly with `gc = true`. **Dependency rule:** a GC module may only depend on modules that require GC; a GC-requiring module in a build without GC errors — *module requires gc, no gc on the job/executable/program* — so GC cannot silently enter GC-free code. **Collector:** conservative (Boehm-style) first. **Borrow checking:** kept (aliasing/races), lifetimes relaxed. **`@delete`:** optional finalizer. |
| 08 | Web/DOM type declarations | **Approved** | **Source:** a first-party `dom` module shipped with the compiler/std, generated once from Web IDL and versioned with releases (no remote-import drift). Host-handle rule proposed (DOM refs are non-owning handles). |

## 2026-10-13 — first review pass

| # | Proposal | Decision | Notes / learned constraints |
|---|----------|----------|------------------------------|
| 01 | MIR core consolidation & thin backends | **Ongoing — no proposal doc** | Already being worked on; covered by `lang/docs/mir-migration-evaluation.md`. Not rejected. Implementation of approved proposals starts **after** this work finishes. |
| 02 | Compiler size & perf budget | **Not a proposal — noted as future concern** | Size <5 MB is already communicated and tracked; current goal is *featureful first*, then reduce size **without losing features**. |
| 03 | Generic impls | **Approved** | Implementation must be extremely fast (no compile-time/memory regression). Needs many **common core-language tests run on all execution paths** (interpreter, TCC, LLVM). Unlocks JSON for generic containers → spawned proposal 14. |
| 04 | Borrow checker & memory-safety closure | **Approved** | Must be extremely fast; **must not slow the existing compiler**. Same no-regression rule as 03. |
| 05 | No-panic effect system | **Needs information** | Not ready to approve. Wants concrete: exact syntax, *why* that syntax, performance analysis, how the effect system works. Prior `effect-system-proposal.md` is stale/untrusted. |
| 06 | Native GC mode | **Needs information** | In principle yes, but wants concrete: what changes, the syntax, how a user writes GC code. "What are we even implementing — that's what should be approved." |
| 07 | JavaScript backend | **Approved** | Backends must stay **thin**; shared code goes into MIR; no loss of performance for the toolchain or any single backend. |
| 08 | Web/DOM type declarations | **Needs information** | Wants: does it keep compilation fast; how do remote (GitHub) modules work; does it affect other backends; how do users/AIs use it. |
| 09 | WASM backend | **Deferred (future)** | No present need; TinyCC is gaining TLS and may add WASM. Revisit when the need appears. |
| 10 | JVM bytecode backend | **Deferred — gated on 06** | Reconsider after the native-GC proposal is approved. |
| 11 | Android + cross-compilation | **Deferred — gated on 06** | Too big now; needs native GC semantics and opt-in settled first. Wants specific syntax/user-facing/perf decisions, not "should we". |
| 12 | Stdlib & ecosystem integration | **Not now — shared IO only (candidate)** | Only the shared IO abstraction (Reader/Stream) is a candidate; split out as proposal 15. |
| 13 | Async/await completion | **Approved** | |
| 14 | Generic std-container JSON serialization | **Approved** (new, from 03) | vector / unordered_map / ordered_map need serializer+deserializer impls; requires generic impls. |
| 15 | Shared IO abstraction (Reader/Stream) | **Candidate — needs review** | Split from 12 per decision. |

## Standing constraints learned this pass

1. **No performance regression.** Any approved feature (generic impls, borrow checker,
   new backends) must be implemented for speed and **must not slow the compiler**
   (compile time, memory, generated-code quality). Shared code belongs in MIR, and
   adding a backend must stay *thin*.
2. **Common core-language tests on every backend.** We need a large body of core-language
   tests that run on interpreter, TCC, LLVM (and future backends) after each feature.
3. **Featureful first, then shrink.** Compiler binary size (<5 MB goal / 5 MB ceiling) is
   tracked but secondary until the feature set is complete; size is then reduced *without
   losing features*.
4. **Native GC gates JVM/Android.** JVM and Android proposals stay deferred until the
   native-GC semantics and opt-in are approved.
5. **WASM is deferred to TinyCC** possibly providing it; not a current direction.
