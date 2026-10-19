# Async/await completion

**Chemical · Proposal 0013**

> Close the last async gap (async closures), settle the LLVM lowering path, and
> retire the library workarounds.

**Status:** Approved · **Area:** Language · runtime · [review index](review.md)

## Goal fit

- **R3** async on every target
- **R0** remove workaround tax

## 1. Problem

Async ships but with workarounds. The remaining worked-around item is **AC** —
async closures are parsed but not lowered. Platform items B15-W, B20, B23–B26,
POSIX-EPOLL, TLS-VT and WIN-IOCP are fixed. On top of that, the docs **disagree**
about the LLVM path: `compiler/mir/README.md`'s top note and
`mir-llvm-migration.md` mark MIR→LLVM as _PLANNED_, while `compiler/CodegenMir.cpp`
(a MIR→LLVM emitter with no AST dispatch) exists and is compiled into `Compiler`.
The active path must be established before relying on either claim.

## 2. Goals & non-goals

- **In scope:** lower async closures; establish/complete the LLVM async path;
  remove library workarounds once the compiler fixes are green across the
  verification matrix.
- **Out of scope:** new async APIs; runtime redesign.

## 3. Proposed approach

1. **Async closures.** Extend the async lowering (`lower_async_function`) to
   closures: represent the closure environment as part of the frame, lower
   captures as frame fields, and emit the same `__poll`/`__drop`/ramp/vtable shape
   used for functions.
2. **LLVM path.** Determine which path is authoritative today; then either finish
   wiring the MIR→LLVM emitter (delete the AST-based lowering) or remove the dead
   emitter. This is shared with proposal 01's "one core, thin backends" goal.
3. **Workaround removal.** Once fixes are green on the full matrix, remove the
   library workarounds documented in `async-remaining-work.md` (they are the
   compatibility contract until then).
4. **Cross-backend.** Ensure the async common tests run on interpreter, TCC and
   LLVM (and the future JS/JVM backends).

## 4. Alternatives considered

- **Leave closures unsupported**: rejected — an inconsistent language surface.
- **Keep both LLVM paths**: rejected — duplicated backends contradict 01.

## 5. Phased milestones

1. Verify current LLVM async path; record the truth in `compiler/mir/README.md`.
2. Lower async closures (frame + captures + poll/drop/vtable).
3. Converge on one LLVM path via 01.
4. Remove library workarounds; run the verification matrix.
5. Common async tests on all execution paths.

## 6. Acceptance criteria

- Async closures compile and run correctly on TCC and LLVM.
- Exactly one LLVM lowering path exists, documented and tested.
- The async suite (`--async`) stays green on TCC and LLVM with full debug info.
- Library workarounds for fixed items are removed without regressions.

## 7. Risks & open questions

- Async on the native GC model (06) and on JS/JVM backends (07/10) — lowering may
  differ per host.
- Debug-info stability through the coroutine split (previously fragile).

## 8. Dependencies

Depends on: 01 (backend consolidation) · Unblocks: clean async across future
backends.

## 9. Evidence & references

- `lang/docs/async-remaining-work.md` (item AC; fixed-item list).
- `compiler/mir/README.md` ("next phase" note) vs.
  `lang/docs/mir-llvm-migration.md` ("PLANNED").
- `compiler/CodegenMir.cpp` (exists; compiled into `Compiler` via
  `CMakeLists.txt`).
