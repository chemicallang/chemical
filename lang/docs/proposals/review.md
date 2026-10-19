# Feature Proposals — Status

**Chemical · Proposals**

> 15 items. Eight proposals are approved with full documents. One (effects) was
> analysed before approval. Deferred items wait on the native-GC decision.

**8 approved · 3 deferred · 1 ongoing · 1 noted · 1 not-now · 1 candidate**

## Second pass — 2026-10-13 (full log: [DECISIONS.md](DECISIONS.md))

- **06 Native GC — approved.** Per-module `gc = true`; GC modules may only depend
  on GC modules (a GC module in a non-GC build errors); conservative Boehm-style
  collector; borrow checking kept, `@delete` optional.
- **08 Web/DOM types — approved.** First-party `dom` module shipped with the
  compiler (generated once from Web IDL, versioned).
- **05 Effects — approved.** Square brackets; v1 effects `panic`, `pure`,
  `blocking`, `unsafe`, `alloc`, `noreturn`; restrictions are per-function (plus a
  module default); effects are declared, never inferred across current-module
  forward calls; `unreachable`/traps are `unsafe`, not `panic`.

## Index

| #  | Proposal                                             | Area                | Depends on | Status                     |
| -- | ---------------------------------------------------- | ------------------- | ---------- | -------------------------- |
| 01 | MIR core consolidation & thin-backend architecture   | Compiler            | —          | Ongoing                    |
| 02 | Compiler size & performance budget                   | Toolchain           | —          | Noted                      |
| 03 | [Generic impls](0003-generic-impls.md)               | Language · generics | —          | Approved                   |
| 04 | [Borrow checker & memory safety](0004-borrow-checker-memory-safety.md) | Language · safety | 03, 05 | Approved |
| 05 | [No-panic effect system](0005-effect-system.md)      | Language · effects  | 04         | Approved                   |
| 06 | [Native GC mode](0006-native-gc-mode.md)             | Memory model        | 01, 04     | Approved                   |
| 07 | [JavaScript backend](0007-javascript-backend.md)     | Backend · web       | 01, 04     | Approved                   |
| 08 | [Web/DOM type declarations](0008-web-dom-types.md)   | Language · types    | 07         | Approved                   |
| 09 | WASM backend                                         | Backend             | 01, 06     | Deferred                   |
| 10 | JVM bytecode backend                                 | Backend · managed   | 06         | Deferred — revisit after 06 |
| 11 | Android target & cross-compilation                   | Target · toolchain  | 06         | Deferred — after 06        |
| 12 | Stdlib & ecosystem integration                       | Libraries           | 03         | Not now                    |
| 13 | [Async/await completion](0013-async-await-completion.md) | Language · runtime | 01      | Approved                   |
| 14 | [Generic std-container JSON serialization](0014-generic-container-serialization.md) | Libraries · JSON | 03 | Approved |
| 15 | [Shared IO abstraction (Reader/Stream)](#15-shared-io-abstraction-reader--stream) | Libraries | — | Candidate |

## All open decisions resolved

Nothing is awaiting a decision. The last open item — the effect system (05) — is
now approved. The use-case catalog behind it is kept at
[0005-effect-system-analysis.md](0005-effect-system-analysis.md) for reference.

## Approved — full documents

- [0003 — Generic impls](0003-generic-impls.md)
- [0004 — Borrow checker & memory-safety closure](0004-borrow-checker-memory-safety.md)
- [0005 — Effect system & no-panic enforcement](0005-effect-system.md)
- [0006 — Native GC mode](0006-native-gc-mode.md)
- [0007 — JavaScript backend](0007-javascript-backend.md)
- [0008 — Web/DOM type declarations](0008-web-dom-types.md)
- [0013 — Async/await completion](0013-async-await-completion.md)
- [0014 — Generic std-container JSON serialization](0014-generic-container-serialization.md)

## Closed / deferred

### 01 — MIR core consolidation

**Ongoing.** Already being worked on, covered by
`lang/docs/mir-migration-evaluation.md`. Approved proposals are implemented after
this work finishes.

### 02 — Compiler size & performance budget

**Noted.** No proposal; the &lt;5 MB budget is tracked. Featureful first, then
shrink without losing features.

### 09 — WASM backend

**Deferred.** No present need; TinyCC may add WASM. Revisit when the need appears.

### 10 — JVM bytecode backend

**Deferred — after 06.** Native GC is now approved; JVM will be reconsidered next.

### 11 — Android target & cross-compilation

**Deferred — after 06.** Returns later as a specific brief (syntax, opt-in,
performance).

### 12 — Standard library & ecosystem integration

**Not now.** Only shared IO is a candidate — see 15.

## Candidate

### 15. Shared IO abstraction (Reader / Stream)

**Candidate.** A `Reader`/`Stream` trait pair plus adapters so `fs`, `net`,
`http`, `compression` and `archive` compose. Everything else in the stdlib
analysis stays deferred. **Review:** approve · reject · changes.

---

Chemical feature proposals · status · 15 items · 8 approved.
Process: [README.md](README.md) · decisions: [DECISIONS.md](DECISIONS.md).
