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
| 3 | **Runs everywhere** | Browser (as JS), WASM, JVM bytecode, Android, and native (C + LLVM). One project, structured to run on every target. |

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
6. **One syntax across divergent platform semantics** — the Kotlin-Multiplatform-like
   problem, harder because Chemical is a systems language. This is the central open
   design question. Target matrix: **native (C + LLVM), WASM, JS/browser, JVM, Android**.

## Design tensions to hold explicitly

1. Safety vs. performance — safety checks must be compile-time / zero-cost on the safe path.
2. One syntax vs. many platforms — same syntax must compile to native, WASM, JS, JVM.
3. Agent-friendliness vs. human legibility — never cross into "only an AI can read this".
4. Compile-time speed vs. convenience — no slow-to-check sugar.
5. Constraint vs. expressiveness — constrain wrong programs rather than expand expression.

## Non-goals

- Not building another Rust; will not copy Rust syntax wholesale.
- Not optimizing for human authoring convenience.
- Not adding type inference / sugar that costs compile time for little value.
- Not normalizing memory-unsafe escape hatches (they stay explicit, scoped, opt-in —
  see the existing `unsafe` marker rules in `AGENTS.md`).

## See also

- Canonical doc: [`lang/docs/language-goals.md`](../../../lang/docs/language-goals.md)
- Safety posture: [`lang/docs/chemical-safety-analysis.md`](../../../lang/docs/chemical-safety-analysis.md)
- Safety plan: [`lang/docs/safety-implementation-plan.md`](../../../lang/docs/safety-implementation-plan.md)
- Panics/effects: [`lang/docs/effect-system-proposal.md`](../../../lang/docs/effect-system-proposal.md)
- Cross-compilation: [`lang/docs/cross-compilation.md`](../../../lang/docs/cross-compilation.md)
- Borrow/lifetime notes: [`lang/docs/notes/SafetyAssessment.md`](../../../lang/docs/notes/SafetyAssessment.md), [`lang/docs/notes/SafetyChecklist.md`](../../../lang/docs/notes/SafetyChecklist.md)
