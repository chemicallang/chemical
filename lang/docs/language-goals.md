# Chemical Language — Goals, Vision & Cross-Platform Strategy

> **Status: Living design document — created October 12, 2026.**
>
> This document states **where the language is going**, not only what exists today.
> Treat every "we will / we want / we plan" statement as a **direction**, and every
> "today / currently / not yet" statement as the **current reality**. When a claim
> conflicts with the source tree, the source tree wins — but the *intent* here should
> still guide design decisions.
>
> **Companion documents:**
> - [`chemical-safety-analysis.md`](chemical-safety-analysis.md) — current safety posture and gaps.
> - [`safety-implementation-plan.md`](safety-implementation-plan.md) — phased, AI-implementable safety work.
> - [`effect-system-proposal.md`](effect-system-proposal.md) — how panics/errors-as-effects may be tracked.
> - [`cross-compilation.md`](cross-compilation.md) — cross-compilation design and goals.
> - [`notes/SafetyAssessment.md`](notes/SafetyAssessment.md) and [`notes/SafetyChecklist.md`](notes/SafetyChecklist.md) — borrow/lifetime tracking notes.
> - [`library-ecosystem-analysis.md`](library-ecosystem-analysis.md) — library gaps.
>
> Agent-facing summary: [`.agents/skills/language_goals/SKILL.md`](../../.agents/skills/language_goals/SKILL.md).

---

## 1. The core shift: Chemical is for AIs **and** humans

Chemical is **not** being developed for humans alone.

AI coding agents have changed the game. Today:

- **Most people no longer write code** — they read it, review it, and direct agents.
- Some people do not read the code at all; they trust the agent and the tests.
- Where humans *do* write code by hand, it is because the code is **extremely
  critical and must be right**.

Chemical is therefore designed around two equal-but-different readers:

| Reader | What they need from the language |
|--------|----------------------------------|
| **AI agents** (primary authors) | Hard constraints that make bad code impossible or hard to write; predictable, checkable semantics; no footguns; clear compiler errors that tell the agent how to fix the code. |
| **Humans** (reviewers, auditors, architects) | Enough readability to get the *gist* of a project at a glance, then dig in with AI assistance. Not maximal expressiveness — maximum *understandability per unit of effort*. |

The goal is **not** to build a language only AIs can understand, and **not** to keep
adding human-oriented syntactic sugar. It is to make correct code **easy to generate**
and **easy to verify**.

### What this changes in practice

- Correctness is enforced by the compiler/type system, not by convention.
- Features are judged by **how hard they make it to write a bug**, not by how pleasant
  they are to type.
- If a feature primarily helps humans *write* code (and costs compile-time or
  semantic complexity), it is a candidate to **drop**.
- If a feature helps agents or reviewers *prove* code correct, it is a candidate to
  **add**.

---

## 2. Non-negotiable requirements for the language to be valuable

These are the four pillars. Everything else is secondary.

### Requirement 0 — Extreme performance

Performance is **the** reason to care. Code is no longer the bottleneck:
maintenance, bug hunting, and new-feature development can largely be done by AI.
What remains scarce and expensive is **runtime performance**.

Consequence: do not trade runtime performance for developer convenience. Compile-time
speed matters too (see §4), but the hard floor is that generated code must be fast —
C/LLVM-class speed, no hidden runtime for safety that the code did not opt into.

### Requirement 1 — Complete memory safety

The language must be **completely memory safe**, so that an AI cannot write code that
drives the program into a memory error.

This is a hard requirement, not a best-effort one. The target model is **Rust's**, so
that agents already trained on the Rust ecosystem can transfer their instincts
(see §3).

### Requirement 2 — Enforceable no-panic policy (per module)

A user must be able to **enforce that no panic is possible in an entire module**. The
compiler must **complain if a function can lead to a panic**.

- This lets a user forbid agents from introducing panics into a program.
- It forces agents to **report errors** (via return values / `Result` / effects)
  instead of aborting the program.
- It is a module-level, opt-in policy: modules that need panic-free guarantees turn it
  on; low-level runtime modules may opt out.

See [`effect-system-proposal.md`](effect-system-proposal.md) for the effect-based
design direction and `std::Result` for the error-reporting idiom.

### Requirement 3 — Run everywhere

A Chemical project should run on **any platform**:

- **Browser** — run as JavaScript in the browser (and separately, compile to **WASM**).
- **JVM** — compile to JVM bytecode.
- **Android** — run as an Android app.
- **Native** — compile natively via the **C** backend and the **LLVM** backend.

An AI should know how to structure a project so that it runs everywhere; the language
and toolchain should make that **really easy**.

This is the **Kotlin Multiplatform** ambition — but harder, because Chemical is a
systems language with native (non-GC) semantics, and different targets have genuinely
different memory models. The unsolved design problem is **one syntax that stays
correct across divergent platform semantics** (see §7).

---

## 3. Relationship to Rust: adopt the memory-safety *model*

Most capable AI agents are already fluent in Rust. To reduce translation errors, Chemical
should adopt Rust-compatible **semantics** for memory safety, so an agent writing
Chemical expects it to behave like Rust in that regard.

### Adopt (semantic familiarity)

- **Ownership + move semantics** (Chemical already has deterministic destructors and
  move tracking).
- A **borrow checker**: one mutable *or* many immutable borrows; references cannot
  outlive their referent; no dangling references.
- **Strict memory safety and type safety** as compile-time errors, not warnings.
- Lifetimes / borrow tracking enforced at compile time, with existing libraries able to
  opt out during migration.

### Keep (Chemical's own syntax — do **not** copy Rust here)

- **Function return types use `:`**, not Rust's `->`:
  ```chemical
  func add(a : int, b : int) : int { return a + b }
  ```
- Chemical's existing syntax is good; we keep it and keep improving it where worthwhile.
- Small syntax differences from Rust are expected. **This is not another Rust.** The
  *safety model* is familiar; the *surface syntax* is Chemical.

### Refuse (features we may never support)

- Features that **consume compilation time for little programming value** — especially
  Rust-style **type inference that resolves a type from later statements** (e.g.
  inferring a vector's element type from how it is used much later). Agents can write
  the type explicitly; faster compiles are worth more than the inference.

Heuristic for rejecting a feature:

> If the primary beneficiary is a human's typing fingers, and the cost is compile-time
> performance or semantic complexity, it is probably not worth adding.

---

## 4. Sharpen the rough edges

Beyond big features, the language must **eliminate the common mistakes agents make**.
This is a continuous effort:

- A bad pattern that an agent writes repeatedly should be made **impossible or
  obviously wrong at compile time** — not documented and left to discipline.
- Compiler diagnostics should be written for an **agent as the reader**: state what is
  wrong, why, and the concrete fix.
- Prefer **fewer ways to express the same thing** where the alternatives invite bugs.
- Every new feature is evaluated against: *does this reduce or increase the space of
  wrong programs an agent can write?*

See the [`diagnostics` skill](../../.agents/skills/diagnostics/SKILL.md) and
[`type_verification` skill](../../.agents/skills/type_verification/SKILL.md).

---

## 5. Readability for humans (without paying for expressiveness)

When a human opens a Chemical project:

- The language should **not** be so alien that only an AI can understand it.
- The human should **get the gist** of what the project is expressing.
- For deep dives, the human relies on AI assistance.

It may be **a little harder** to read than a language optimized purely for human
expressiveness — that is an accepted trade. We are not optimizing for expressiveness;
we are optimizing for **constraint + verifiability**, while staying legible enough to
review.

When an AI reads the code, it should be able to work out **how the code works and what
it emits**. Agents are good at analyzing large, complex code — so we lean on that
strength and let the DSL-style sugar go.

---

## 6. Current blockers on the path to "runs everywhere"

These are the concrete gaps called out for the cross-platform goal. They are **not yet
solved** unless noted.

### 6.1 Universal components and `#js` emit raw, unchecked JavaScript

The universal-component pipeline and the `#js` macro currently emit **raw JavaScript
with no type checking** — values go straight to the browser, and the generated code
causes bugs. There is no type-checked boundary between Chemical and the DOM.

*Direction:* make the client/JS side type-checked, or at minimum raise the generated
code to a checked, declarative layer. See the [`universal` skill](../../.agents/skills/universal/SKILL.md)
and [`macro_code_gen` skill](../../.agents/skills/macro_code_gen/SKILL.md).

### 6.2 No TypeScript-equivalent web/DOM type definitions

Chemical does not have the equivalent of TypeScript's **web platform definitions** —
e.g. the set of methods and properties available on a DOM element. We also have no
system for declaring such APIs at scale (there are enormous numbers of declarations).

*Direction:* design a **declaration/binding system** for large external API surfaces,
then ingest the standard web DOM/WHATWG definitions. See
[`library-ecosystem-analysis.md`](library-ecosystem-analysis.md).

### 6.3 No JavaScript backend — Chemical cannot run on the web

There is **no JS backend**, so Chemical cannot be translated to run in the browser.
(WASM is also planned, but browser execution specifically requires running **as JS**,
not only as WASM.)

### 6.4 Memory model vs. the web

Even with a JS backend, Chemical's memory restrictions and borrow checking make web
code harder to write, because the DOM and browser APIs are inherently reference-heavy
and GC-managed.

### 6.5 GC platforms vs. native semantics

On GC platforms (e.g. the JVM), the language's native-oriented semantics (ownership,
deterministic destruction, borrow checking) do not map cleanly. We need the language to
be designed so that **one source syntax works across all platforms**, even though the
platforms have fundamentally different memory models.

### 6.6 One syntax across divergent platform semantics

This is the Kotlin-Multiplatform-like challenge, but with an extra difficulty: those
platforms required **different syntax** to cope with semantic differences. Chemical
wants **one syntax** that is flexible enough to express programs for every target. This
is the central open design problem for Requirement 3.

> WASM is also on the roadmap. The target matrix is: native (C + LLVM), **WASM**,
> **JS/browser**, **JVM bytecode**, **Android**.

---

## 7. Design tensions to keep in mind

Agents and humans proposing features should hold these tensions explicitly:

1. **Safety vs. performance** — safety must be compile-time / zero-cost on the safe
   path; no runtime tax for code that didn't opt in.
2. **One syntax vs. many platforms** — the same syntax must compile to native, WASM,
   JS, and JVM despite different memory models.
3. **Agent-friendliness vs. human legibility** — optimize for generating and verifying
   correct code, but never cross into "only an AI can read this."
4. **Compile-time speed vs. convenience** — features that slow compilation for
   little verification value are not worth it.
5. **Constraint vs. expressiveness** — when in doubt, constrain the set of wrong
   programs rather than expand the expressive surface.

---

## 8. Non-goals

- We are **not** building another Rust, and will not copy Rust syntax wholesale.
- We are **not** optimizing the language for human authoring convenience.
- We are **not** adding type inference / sugar that costs compile time for little
  verification value.
- We are **not** accepting memory-unsafe escape hatches as the normal path (they remain
  explicit, scoped, and opt-in — see the existing `unsafe` marker).

---

## 9. How to apply this document

When working on the language, compiler, libraries, or toolchain:

1. **Check the requirements** (§2) — does the change move us toward, or away from,
   performance / memory safety / no-panic enforcement / running everywhere?
2. **Prefer constraints over sugar** (§4) — make wrong programs harder to write.
3. **Keep the Rust semantic familiarity** (§3) for memory safety, but keep Chemical
   syntax.
4. **Keep the human in the loop** (§5) — readable enough to review.
5. **Treat the blockers in §6 as the current critical path** toward Requirement 3.

If a design decision needs to trade one pillar against another, document the trade
explicitly in the relevant `lang/docs/` document so future agents inherit the reasoning.
