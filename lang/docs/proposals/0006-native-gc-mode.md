# Native GC mode

**Chemical · Proposal 0006**

> An explicit, per-module garbage-collected memory model that still ships a real
> native binary with an embedded collector — no hosted VM.

**Status:** Approved · **Area:** Memory model · [review index](review.md)

## Goal fit

- **R3** native GC, no hosted VM
- **R0** opt-out pays nothing
- **R1** memory safety kept

## 1. Problem

"Runs everywhere" includes a Go-like native mode: ignore allocation and still
produce a native binary with an embedded collector. Today native code is
deterministic/owned; JVM/JS give GC but are hosted. The semantics and boundary
were open; this proposal fixes them (see Decisions).

## 2. Decisions (from review)

1. **Explicit opt-in per module:** a module turns GC on with `gc = true`. No GC is
   active by default.
2. **Consistent dependency rule:** a GC module may only depend on modules that also
   require GC. If a GC-requiring module is included in a build where GC is not
   enabled, the compiler **errors**: _"module requires gc, no gc on the
   job/executable/program"_. This prevents GC code from silently entering GC-free
   code, and warns authors pulling in a GC module.
3. **Collector:** conservative (Boehm-style) first — a small C collector linked
   into the executable; non-moving; scans registers/stack/heap. Precise/moving is a
   later extension.
4. **Borrow checking is kept** for aliasing and data races; GC removes the
   lifetime/use-after-free pressure (the collector guarantees memory safety).
5. **`@delete` becomes optional** (a best-effort finalizer). It must **not** be
   relied on for OS resources (files/sockets/locks) — those keep explicit,
   deterministic cleanup.

## 3. Proposed approach

1. **Module flag & dependency check.** Parse `gc = true` in `chemical.mod`; at
   build-graph construction, verify GC consistency across the dependency closure
   and emit the error above on violation.
2. **Allocation & type rules.** In a GC module, values allocate on the GC heap and
   are referenced as managed references; owned values and the `unsafe` escape hatch
   remain available. The boundary between GC and owned values is explicit (copy at
   the boundary).
3. **Collector runtime.** Vendor a proven conservative collector, linked into the
   produced executable (no external VM). Roots: registers, stack, and registered
   globals. Safe points at allocation and at loop back-edges/calls.
4. **MIR / backends.** Allocation, store barriers (if needed), and finalization are
   represented in MIR and shared across the C and LLVM emitters (thin backends —
   proposal 01).
5. **Borrow-checker integration.** Extend proposal 04: for GC values, lifetime
   restrictions are relaxed, but aliasing/race restrictions remain. Guarantees must
   not leak into owned code.
6. **Finalizers.** `@delete` on a GC type becomes a best-effort finalizer with
   defined scheduling; OS resources use explicit `close`/scope instead.
7. **Cost.** GC-free binaries must be unaffected (no collector linked, no
   barriers). Track binary size of the embedded collector against the toolchain
   budget.

## 4. Alternatives considered

- **Whole-program GC** (no mixing): simpler, but cannot host owned code in the same
  binary — rejected in favour of per-module.
- **Per-type `@gc`**: too fine-grained, needs complex containment/crossing rules —
  deferred.
- **Precise moving collector first**: better pause/compaction but much more work —
  deferred behind conservative.
- **Rely on JVM/browser for GC**: rejected — R3 requires a native binary.

## 5. Phased milestones

1. Semantics spec + `gc = true` flag + dependency-consistency diagnostics.
2. Vendor the conservative collector; link it; route allocation through it in MIR.
3. Root registration (stack/registers/globals) and safe points.
4. `@delete` → finalizer scheduling.
5. Borrow-checker relaxation for GC values (with 04).
6. Common core-language tests on the GC path; confirm opt-out is zero-cost.
7. Size accounting; decide on a later precise/moving collector.

## 6. Acceptance criteria

- A `gc = true` module builds, runs, and reclaims memory natively — no hosted VM.
- Mixing GC and non-GC modules in one build errors with the specified diagnostic.
- `@delete` is optional in GC modules; OS resources remain deterministically
  closed.
- GC-free builds are unaffected in behaviour, size (as far as possible), and
  performance.
- Tests run on TCC and LLVM.

## 7. Risks & open questions

- Register/stack scanning correctness; conservative retention (false pointers) and
  its memory overhead.
- Threading: per-thread vs global, stop-the-world vs incremental.
- Async frames (13) as GC roots.
- What exactly "the boundary" permits between owned and GC values.
- Collector binary-size contribution vs the 5 MB budget.

## 8. Dependencies

Depends on: 01 (MIR), 04 (borrow checker) · Unblocks: 10 (JVM) and 11 (Android)
reconsideration; the GC axis of R3.

## 9. Evidence & references

- `lang/docs/language-goals.md` §6.7 + tension 6.
- `lang/docs/proposals/DECISIONS.md` (second pass — the five decisions above).
- `lang/docs/mir-migration-evaluation.md` (MIR as the shared lowering substrate).
