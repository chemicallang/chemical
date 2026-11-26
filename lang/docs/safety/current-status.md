# Chemical safety — current status

This is the authoritative, short current-status entry point for the safety
program. The live per-job evidence table is
[`../safety-sharpen/STATUS.md`](../safety-sharpen/STATUS.md); the plan is
[`../safety-sharpen-plan.md`](../safety-sharpen-plan.md). Full design detail and
the chronological decision record live in [`design.md`](design.md) and
[`history.md`](history.md).

`history.md` is the pre-split monolithic document (formerly
`safety-implementation-plan.md`); it is kept verbatim for decision archaeology.
The source tree wins over any status note.

## Non-negotiable requirements

0. Extreme performance.
1. Complete memory safety (an agent must not cause a memory error).
2. Enforceable no-panic policy (per function / per module).
3. Runs everywhere (C/TinyCC, LLVM, JS/browser, WASM, JVM, Android, native GC).

## Enforced today

| Area | Status |
|---|---|
| Diagnostics/audit suites | Coded `SAFE-*` errors; negative/safety suites; whole-tree audit gate |
| Unsafe / FFI | Raw-pointer deref/projection/arithmetic/delete, `unsafe func` calls, `@extern`, inline asm and raw reinterpretation require an `unsafe` boundary (`SAFE-UNSAFE-*`, `SAFE-PROV-001`, `SAFE-FFI-*`); a public function taking a raw pointer must be `unsafe func` |
| Init / moves / drops | Definite-assignment (`SAFE-INIT-001`), move/drop tracking in MIR, field/element moves, consuming `self`; unsupported shapes reject with `SAFE-MOVE-004` |
| Borrows / lifetimes | Regions, NLL loans, storage lifetimes, return regions, variance, temporary extension, provenance, HRTB call regions; aggregate-wrapped reference results carry the receiver loan |
| `Send` / `Sync` | Auto traits, explicit/negative impls, conditional generic impls; guards are `!Send`; shared-reference `Send` checks nested `!Sync` |
| Validity / runtime | Bounds, overflow/div/shift, float→int, allocation/layout, packed/transparent, abort-only panic |
| Async | `Pin`/Waker/frame loans; `FutureHandle<'a, T>` frame lifetime; `SAFE-LIFE-005` retaining-API escapes; `spawn_send` `'static` check |
| Effects | `[no_panic]` panic-effect slice (direct callees, function pointers, `dyn`, throws); `[no_alloc]`/`[no_unsafe]`/`[no_blocking]`/`[pure]` are opt-in Phase 3 additions |
| Safe APIs | Safe-by-default library names; raw forms under `<lib>::raw` / `<lib>::ffi`; owning handles/RAII; fallible container/string/alloc forms |

## Known open items

- Full generic/async/interface effect propagation and the module-level effect
  default policy (P3.1).
- Fallible `string`/container/thread-pool/async constructor coverage (P3.2).
- A few move/borrow shapes that are conservatively rejected; only additive
  fixes land on the stable branch (P3.3, `NONBREAKING_DEFERRED.md`).
- Windows/macOS/GUI execution is environment-gated; see
  [`../safety-sharpen/TARGETS.md`](../safety-sharpen/TARGETS.md).
- Compiler size: hard ceiling 5,000,000 B, goal 4,000,000 B (P3.6,
  [`../safety-sharpen/metrics.json`](../safety-sharpen/metrics.json)).

## Recorded green baseline

Last recorded full matrix (2026-11-22, release-readiness wave): TCC main
2495/2495, negative 902/902, server 31/31, libs 904/904, safety 193/193,
plugins 1305/1305, async 84/84, interpret 1938/1938, `MIRTests` 4749 checks.
Re-run the suites for the current worktree; treat these as the last reviewed
counts, not a promise about uncommitted code.
