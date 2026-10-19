# Borrow checker & memory-safety closure

**Chemical · Proposal 0004**

> Turn the compiler's existing move/ownership _tracking_ into _enforcement_:
> use-after-move and aliasing become compile errors — fast.

**Status:** Approved · **Area:** Language · safety · [review index](review.md)

## Goal fit

- **R1** complete memory safety
- **R0 / TC** no compiler slowdown
- **R2** shares effect machinery

## 1. Problem

The goals adopt Rust-familiar memory safety (ownership, move semantics, a borrow
checker) but there is **no borrow checker in the compiler** — no
`BorrowCheck*`/ownership pass under `compiler/` or `ast/` (verified). What exists
is tracking, the explicit `unsafe` marker, and optional runtime checks. The safety
plan itself says the infrastructure is complete at the _tracking_ level and
enforcement is what is missing. So use-after-move, double-drop, and conflicting
aliasing bugs that the goals say must be compile errors are currently accepted.

## 2. Goals & non-goals

- **In scope:** compile-time enforcement of moves/ownership (use-after-move,
  double-drop), a borrow checker (one mutable _xor_ many immutable borrows; no
  reference outliving its referent), per-module safety levels, diagnostics +
  negative tests, **no compiler slowdown**.
- **Out of scope:** runtime bounds/overflow checks (separate flags), the native GC
  model (06), full Rust-NLL sophistication if it costs compile time.

## 3. Proposed approach

1. **Placement & parallelism.** A dedicated pass after type verification, before
   codegen. Run it **per function in parallel** (matching the existing
   file/per-function parallelisation) with an arena of pooled state, so it adds
   little wall-clock time.
2. **Ownership first.** Track move/initialisation state per place (extending the
   existing move tracking used by codegen): reading or dropping a moved-from value
   is an error. A full assignment is (re)initialisation; `unsafe` remains the
   explicit escape hatch.
3. **Borrow checking second.** Track loans (shared/exclusive) per place and detect
   conflicting simultaneous loans and escapes. Start with local flow-sensitive
   analysis (live intervals), not full NLL, and keep it linear-ish; it must be
   extendable later.
4. **Lifetimes.** Begin with elision + local inference; require explicit annotation
   only where inference fails. Avoid Rust-style inference-from-later-usage (a
   stated non-goal).
5. **Module safety levels.** Follow the safety plan's opt-out architecture
   (`chemical.mod` flag): std and existing libraries opt out; new modules are
   checked by default. This guarantees zero regression for existing code.
6. **Performance gate.** Benchmarks in CI; a regression fails the change. This is a
   hard requirement from the review.

## 4. Alternatives considered

- **Runtime checks instead of static enforcement**: rejected — R0 and it still lets
  the bug ship.
- **No borrow checking**: rejected — violates R1.
- **Full NLL from day one**: likely too slow/complex now; kept as a later
  extension.

## 5. Phased milestones

1. Ownership/move enforcement + diagnostics (use-after-move, double-drop).
2. Borrow checking (alias conflicts + escape), per-function, parallel.
3. Lifetime elision/inference and annotations where required.
4. Per-module safety levels + std/library opt-out.
5. Negative/positive tests + **common core-language tests on interpreter, TCC,
   LLVM**.
6. Performance harness + no-regression gate.

## 6. Acceptance criteria

- Use-after-move, double-drop, and conflicting-alias programs are rejected with
  actionable diagnostics; correct programs are unaffected.
- Existing suites pass unchanged (opt-out for legacy modules).
- No measurable compile-time or peak-memory regression (documented budget).
- Common core-language tests green on interpreter, TCC and LLVM.
- DOM-heavy code has a documented, workable story (see open questions).

## 7. Risks & open questions

- **DOM ergonomics:** strict aliasing makes web code harder (a stated tension).
  Need a per-module relaxation or a documented pattern before 08.
- **GC interaction (06):** does an opt-in GC mode relax borrowing, and how without
  leaking guarantees into owned code?
- False positives / learning curve for AI authors; diagnostic quality is critical.
- Keeping it fast on large generic instantiations.

## 8. Dependencies

Depends on: 03 (generics affect place/move analysis), 05 (shares enforcement
machinery) · Unblocks: 08 (safe web code), R1.

## 9. Evidence & references

- Verified absence: no `BorrowCheck`/`borrow_check`/`ownership` in `compiler/`,
  `ast/`.
- `lang/docs/chemical-safety-analysis.md` ("infrastructure complete at tracking
  level; enforcement missing").
- `lang/docs/safety-implementation-plan.md` (opt-out architecture, negative-test
  framework, phases).
- `lang/docs/proposals/DECISIONS.md` (no-slowdown constraint).
