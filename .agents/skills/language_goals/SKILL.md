---
name: Language Goals & Vision
description: The Chemical language's goals and design direction — AI-first + human-readable, Rust-familiar memory safety (borrow checking), strict type safety, module-level no-panic enforcement, dropping compile-time-expensive sugar (Rust-style type inference), and the cross-platform "runs everywhere" ambition (native C/LLVM, WASM, JavaScript/browser, JVM bytecode, Android). Load when making language-design or roadmap decisions, choosing whether a feature fits, working on memory-safety/borrow-checking direction, the no-panic policy, JS/WASM/JVM backends, web/DOM type declarations, or when a task's correctness depends on the language's stated goals.
---

# Chemical Language — Goals & Vision

This skill is the agent-facing summary of the language's direction. The
**canonical, detailed** document is [`lang/docs/language-goals.md`](../../../lang/docs/language-goals.md) —
read it when you need the full reasoning. This file tells you the headline rules and
where to look.

> **These are goals, not a status report.** Some items are designed and partly built;
> others are aspirational and not implemented. When a claim conflicts with the source
> tree, the source tree wins — but the *intent* here should still steer design choices.

## The core shift

Chemical is developed for **AIs and humans**, not humans alone.

- Most people no longer *write* code; they read/review and direct agents. Some don't
  even read it. Hand-written code is reserved for **critical code that must be right**.
- **AI agents are primary authors.** The compiler must make wrong code hard or
  impossible to write, with diagnostics written for an agent to act on.
- **Humans are reviewers/auditors.** They must get the *gist* of a project and then dig
  in with AI assistance. The language may be a little harder to read than a
  human-expressiveness-first language — that is an accepted trade. It must **not**
  become a language only AIs can understand.
- **Optimize for constraint + verifiability, not expressiveness.**

## The four non-negotiable requirements

A language is only valuable if it satisfies all four:

| # | Requirement | Meaning |
|---|-------------|---------|
| 0 | **Extreme performance** | Code is no longer the bottleneck (AI handles maintenance/features); runtime performance is what matters. Never trade runtime speed for convenience. |
| 1 | **Complete memory safety** | An AI must not be able to write code that causes a memory error. Hard requirement. |
| 2 | **Enforceable no-panic policy** | A module can forbid panics entirely; the compiler errors if any function can panic. Forces agents to *report* errors (via `Result`/effects) instead of aborting. |
| 3 | **Runs everywhere** | Browser (as JS), WASM, JVM bytecode, Android, native (C + LLVM), **and native with GC** (C + LLVM binary with an embedded collector — no hosted VM). One project, structured to run on every target. |

**Implementation status (2026-10-31).** “Complete memory safety” is the
requirement, not a claim that parity is finished. The enforced surface (M0, M3,
and the enforceable M4/M5/M6 rules) is green, but the open gaps include
`FutureHandle<'a, T>`/frame-witness expressiveness (direct borrow tracking,
`SAFE-LIFE-005`, and the `'static`/elided frame lifetime landed; full call-site
inference remains), the full target-aware C ABI (LLVM now
rejects unsupported targets instead of miscompiling), `dyn` having no
auto-trait bounds, and the partially implemented `Fn`/`FnMut`/`FnOnce` callable
model (call-once/mutable-receiver kinds and `FnOnce` capture moves with
per-capture drop flags, field-stored `FnOnce` calls, callable bounds, generic
`Fn`/`FnMut`/`FnOnce` call syntax, and `FnMut` receiver-chain checks exist;
inferred once lambdas, capturing generic lambda inference, and async closures
remain). Load
the `safety` skill and read `lang/docs/safety-implementation-plan.md` §4.5
before making safety/roadmap claims.

**Toolchain constraint (equally binding, §2 of the doc).** The non-LLVM release compiler
(`TCCCompiler` / `chemical`, TinyCC-based) must be **< 4 MB ambitiously, hard ceiling
5 MB — never more** as the steady state. It must **still support the whole language and
every target it owns** (C/TinyCC, JVM, JS; LLVM is the separate heavyweight) and be
**really performant** (compile time, memory, and generated-code quality improve
*together*). How: **move heavy, non-essential machinery into plugins/libraries** (CBI,
optional libs, analyzers, heavyweight backends); keep everything *essential* in the core;
**shrink shared library/runtime code over time**. The **first** release may be much larger
(7–15 MB) to ship fast; **after that**, size + compile time + memory are reduced together
until the ceiling is met. Never drop a capability to hit the number — move it out.
**No regression from new features:** every approved feature (generic impls, borrow checker,
a new backend) must be implemented for speed and must **not** regress compile time, peak
memory, or generated-code quality. Shared machinery belongs in **MIR**; a new backend must
stay **thin**.

## Relationship to Rust (important for correctness)

Many agents are already fluent in Rust, so Chemical deliberately adopts **Rust-familiar
memory-safety semantics**:

- **Adopt:** ownership + move semantics, a **borrow checker** (one mutable *or* many
  immutable borrows; no references outliving their referent), strict memory + type
  safety enforced as compile-time errors.
- **Keep Chemical syntax:** function return types use `:` **not** `->`
  (`func add(a : int, b : int) : int`). Small syntax differences from Rust are expected.
  **This is not another Rust** — the *safety model* is familiar, the *surface* is
  Chemical.
- **Refuse:** features that cost compile time for little verification value —
  especially **Rust-style type inference from later usage** (e.g. inferring a vector's
  element type from how it is used much later). Agents can spell the type out; fast
  compiles are worth more.

## Rejecting a feature (heuristic)

> If the primary beneficiary is a human's typing fingers, and the cost is compile-time
> performance or semantic complexity, it is probably not worth adding.

Prefer making **wrong programs harder to write** over adding expressive surface. Fewer
ways to say the same thing is usually better.

## Cross-platform blockers (the current critical path)

These are the known gaps toward Requirement 3 — mostly **unsolved** today:

1. **`#js` / universal components emit raw, unchecked JavaScript** — values go straight
   to the browser and cause bugs. See the [`universal`](../universal/SKILL.md) and
   [`macro_code_gen`](../macro_code_gen/SKILL.md) skills.
2. **No TypeScript-equivalent web/DOM type definitions** — no declarations system for
   the large external API surface (DOM methods, WHATWG APIs). See
   [`lang/docs/library-ecosystem-analysis.md`](../../../lang/docs/library-ecosystem-analysis.md).
3. **No JavaScript backend** — Chemical cannot currently be translated to run on the
   web. (WASM is also planned, but browser execution specifically needs **JS**.)
4. **Memory model vs. the web** — borrow checking / ownership make DOM-heavy code
   harder, since the browser is reference-heavy and GC-managed.
5. **GC platforms vs. native semantics** — on the JVM, native-oriented ownership and
   deterministic destruction do not map cleanly.
6. **Native GC mode (§6.7 of the doc, proposal `0006` — approved)** — a mode where the user
   ignores allocation/free and the collector is linked **into a native C/LLVM executable**
   (Go's value proposition), not a hosted VM. **Decided:** explicit per-module opt-in
   (`gc = true`); a GC module may depend only on GC modules, and a GC module in a non-GC
   build errors (*module requires gc, no gc on the job/executable/program*); conservative
   (Boehm-style) collector first; borrow checking kept (aliasing/races) with lifetimes
   relaxed; `@delete` becomes an optional finalizer. Constraints: opt-in only, zero tax on
   owned code, still a native binary, deterministic cleanup for OS resources, memory safety
   preserved. Gates the JVM/Android targets.
7. **One syntax across divergent platform semantics** — the Kotlin-Multiplatform-like
   problem, harder because Chemical is a systems language. This is the central open
   design question. Target matrix: **native-owned (C + LLVM), native-GC (C + LLVM),
   WASM, JS/browser, JVM, Android**.
8. **Generic impls are not supported (language-capability blocker, §6.8 of the doc)** —
   you cannot write a parameterized implementation (`impl<T> Trait for MyType<T>`) at
   top level. `impl <T> I for S<T>` is rejected (`don't declare generic parameters`)
   and `impl Future<T> for FutureHandle<T>` does not resolve its methods
   (`unresolved child 'poll'`, verified). The only working form is a **nested impl**
   inside the generic type's body, which is instantiated with the containing type.
   Consequence: generic types cannot participate uniformly in protocols, so generic
   abstractions (iterators, conversions, operators, serialization) must be duplicated
   per instantiation. Direction: monomorphize the impl like generic functions.
   **Status (2026-10): approved** — proposal `0003-generic-impls`; blocks
   `0014-generic-container-serialization` (JSON for generic std containers).

## Design tensions to hold explicitly

1. Safety vs. performance — safety checks must be compile-time / zero-cost on the safe path.
2. One syntax vs. many platforms — same syntax must compile to native, WASM, JS, JVM.
3. Agent-friendliness vs. human legibility — never cross into "only an AI can read this".
4. Compile-time speed vs. convenience — no slow-to-check sugar.
5. Constraint vs. expressiveness — constrain wrong programs rather than expand expression.
6. Deterministic destruction vs. GC convenience — one syntax must serve owned/`@delete`
   and an opt-in native GC model without leaking the GC model's relaxed guarantees into
   owned code or making GC the default (§6.7).
7. Small core vs. full capability — keep the non-LLVM compiler under 5 MB (goal 4 MB) by
   moving non-essential machinery to plugins/libraries, never by dropping features or
   backends; size, compile time, and memory improve together (§2 toolchain constraint).

## Non-goals

- Not building another Rust; will not copy Rust syntax wholesale.
- Not optimizing for human authoring convenience.
- Not adding type inference / sugar that costs compile time for little value.
- Not normalizing memory-unsafe escape hatches (they stay explicit, scoped, opt-in —
  see the existing `unsafe` marker rules in `AGENTS.md`).
- Not shipping a "lite" compiler: the small non-LLVM compiler supports the full language
  and every target it owns; weight moves to plugins/libraries, capabilities do not.
- Not accepting a permanently large compiler: a 7–15 MB first release is a starting point,
  then size + compile time + memory are reduced together to the ≤ 5 MB ceiling.

## See also

- Canonical doc: [`lang/docs/language-goals.md`](../../../lang/docs/language-goals.md)
- Safety skill: [`safety`](../safety/SKILL.md)
- Historical safety research (superseded): [`lang/docs/chemical-safety-analysis.md`](../../../lang/docs/chemical-safety-analysis.md)
- Safety plan: [`lang/docs/safety-implementation-plan.md`](../../../lang/docs/safety-implementation-plan.md)
- Panics/effects: [`lang/docs/effect-system-proposal.md`](../../../lang/docs/effect-system-proposal.md)
- Cross-compilation: [`lang/docs/cross-compilation.md`](../../../lang/docs/cross-compilation.md)
- Borrow/lifetime notes: [`lang/docs/notes/SafetyAssessment.md`](../../../lang/docs/notes/SafetyAssessment.md), [`lang/docs/notes/SafetyChecklist.md`](../../../lang/docs/notes/SafetyChecklist.md)
