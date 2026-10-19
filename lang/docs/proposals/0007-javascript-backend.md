# JavaScript backend

**Chemical · Proposal 0007**

> A thin MIR → JS emitter, so Chemical runs in the browser as real JS — without a
> mountain of per-backend code.

**Status:** Approved · **Area:** Backend · web · [review index](review.md)

## Goal fit

- **R3** browser as JS
- **R0 / TC** thin backend, no toolchain slowdown

## 1. Problem

There is no JavaScript backend. The browser target today is `#js`/universal macros
emitting raw unchecked JavaScript (an explicit goals blocker). Requirement 3 says
Chemical runs in the browser as JS. New backends must be **thin**: adding one must
not add a mountain of code, and shared logic belongs in MIR — without slowing the
toolchain or any single backend.

## 2. Goals & non-goals

- **In scope:** MIR→JS emission for the whole language; a runtime ABI for Chemical
  values; closures; async; interop with the existing universal/`#js` pipeline;
  running the common core-language tests in JS.
- **Out of scope:** DOM type declarations (08); replacing the existing SSR
  pipeline.

## 3. Proposed approach

1. **Thin by construction.** Depend on the shared MIR services that proposal 01
   establishes (types, layout, declarations, naming/ABI, opcode metadata). The JS
   emitter is an opcode→JS-text mapping over the shared traversal, mirroring the C
   emitter's structure — not a second compiler front end. Anything shared goes
   _into MIR_, never copied per backend.
2. **Portability classification.** Drive emission from `MIRPortability`: portable
   opcodes map directly; native-only opcodes are lowered to a JS runtime helper or
   rejected with a clear diagnostic.
3. **Value model / runtime ABI.** Define representations for numbers, strings,
   structs, vectors/maps, `Option`/`Result`, interfaces/vtables, and function
   values. Fix the ABI once so generated modules interoperate.
4. **Memory semantics.** Map owned/deterministic semantics onto the host GC; define
   where `@delete` becomes a no-op / explicit dispose, and how it interacts with
   the borrow checker's model.
5. **Closures & async.** Emit closures and the async lowering to JS
   (Promises/async functions), consistent with the MIR async plan (13).
6. **Integration.** Emit ES modules so the output plugs into the universal bundle's
   `window.$_uc_h`/dispatch model; keep the escape hatch to hand-written `#js`.

## 4. Alternatives considered

- **Source-to-JS (no MIR)**: rejected — another front-end-shaped backend, exactly
  what we are removing.
- **WASM-only browser story**: rejected for now — WASM is deferred (09); JS is the
  direct target.
- **Emit via an external transpiler**: rejected — portability and size.

## 5. Phased milestones

1. Runtime ABI + value model (shared with MIR where possible).
2. Scalars, control flow, functions.
3. Structs/unions/variants, strings, vectors/maps.
4. Interfaces/vtables/dispatch.
5. Closures; then async.
6. ES-module emission + universal interop.
7. Common core-language tests in JS; performance/size validation.

## 6. Acceptance criteria

- The common core-language test suite runs in a JS engine and passes.
- A real Chemical program (not just a fixture) builds and runs in a browser.
- The JS emitter stays thin: it reuses shared MIR services and adds no
  AST-dispatch duplication.
- Adding the backend does not regress toolchain compile time/memory for other
  backends.

## 7. Risks & open questions

- How owned/deterministic semantics map onto a GC host, and where the borrow
  checker relaxes.
- Numeric representation (JS doubles vs. integer widths) and overflow semantics.
- Module/package emission and cross-module ABI stability.
- Keeping generated JS readable enough to debug.

## 8. Dependencies

Depends on: 01 (thin-backend substrate), 04 (ownership model) · Unblocks: 08 (ask:
typed DOM interop).

## 9. Evidence & references

- No JS backend under `compiler/backend/` (verified).
- `MIRPortability` in `compiler/mir/MIRInstruction.cpp` / `MIRTypes.h`.
- `lang/docs/proposals/DECISIONS.md` (thin-backend + no-regression constraint).
- AGENTS.md blocker: `#js` emits raw unchecked JavaScript.
