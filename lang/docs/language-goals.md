# Chemical Language — Goals, Vision & Cross-Platform Strategy

> **Status: Living design document — created October 12, 2026.**
>
> This document states **where the language is going**, not only what exists today.
> Treat every "we will / we want / we plan" statement as a **direction**, and every
> "today / currently / not yet" statement as the **current reality**. When a claim
> conflicts with the source tree, the source tree wins — but the *intent* here should
> still guide design decisions.
>
> **Revision (October 13, 2026):** Requirement 3 now explicitly includes a **native
> GC mode** — one syntax that can compile to a **native C/LLVM executable with an
> embedded garbage collector** (the Golang value proposition: automatic memory
> management, no hosted VM), alongside the existing owned / deterministic-destruction
> model. It is a direction with genuinely open questions; see §6.7.
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

These are the four pillars. Everything else is secondary — except the **toolchain
constraint** below, which is equally binding: a compiler that is huge, slow, or
non-portable undermines all four pillars at once.

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
- **Native with GC** — compile to a **native executable** (C + LLVM) whose heap is
  managed by a garbage collector compiled/linked **into the binary**. The user does
  not allocate or free by hand, and the program does **not** run on a hosted runtime
  (no JVM, browser, WASM VM, or interpreter in the loop). This is a separate axis
  from the VM targets: it is Golang's "automatic memory management, native binary"
  mode. See §6.7.

An AI should know how to structure a project so that it runs everywhere; the language
and toolchain should make that **really easy**.

This is the **Kotlin Multiplatform** ambition — but harder, because Chemical is a
systems language with native (non-GC) semantics by default, and different targets have
genuinely different memory models. The unsolved design problem is **one syntax that
stays correct across divergent platform semantics** (see §7), and — new — **one syntax
that can be either owned-with-deterministic-destruction or GC-managed, on the same
native backend** (§6.7).

### Toolchain constraint — the compiler must be small, fast, and everywhere

The pillars above are only real if the **compiler that delivers them** is itself small,
fast, and portable. This is a binding constraint on the toolchain, not a nice-to-have: a
bloated or slow compiler taxes Requirement 0, and a compiler that cannot reach a platform
breaks Requirement 3.

**Hard size budget (the non-LLVM release).** The released compiler that does **not** link
LLVM — the TinyCC-based `TCCCompiler` / `chemical` artifact — must stay **under 4 MB
ambitiously**, and may go **up to 5 MB**. **5 MB is the absolute ceiling**: not more than
that, ever, as the steady state. 4 MB is the goal; 5 MB is the wall. (The LLVM-linked
`Compiler` is dominated by LLVM/Clang/LLD and is tracked by delta only.)

**Small, but not "lite".** The small compiler must **support the whole language and all
our language goals** — every target it owns (C/TinyCC, JVM, JS; LLVM is the separate
heavyweight), the full semantics, and the same correctness. Smallness must come from
removing **non-essential weight**, never from dropping features, backends, or checks.

**Also really performant.** Compile time, peak memory, and generated-code quality are all
part of "small". The compiler must be fast and memory-efficient, and the code it generates
must be fast (Requirement 0). Binary size, throughput, and memory efficiency are improved
**together**, not traded against each other.

**How this is achieved — the strategy.**

1. **Separate heavy, non-essential machinery into libraries and plugins.** Anything not
   required for the core pipeline (parse → symres → typecheck → lower → emit) lives
   *outside* the core compiler and is pulled in only when used: compiler plugins/CBI,
   optional libraries, analyzers/tooling, and heavyweight backends. **Everything essential
   stays in the compiler** — the core pipeline is never a plugin.
2. **Shrink the library code, continuously.** The support code shared into the compiler
   (runtime, `std`, embedded helpers) is a primary size lever and must get **smaller over
   time**, not larger.
3. **Deliberate two-phase release.** The **first** release may be *much* larger — on the
   order of **7 MB, or even ~15 MB** — to ship capability fast and prove the pipeline.
   That is a temporary state, not the destination. **After that**, a focused effort
   reduces **binary size, compile time, and memory together**, iterating down until the
   5 MB ceiling (goal: 4 MB) is met.

**Non-negotiable consequences**

- 5 MB is a hard release ceiling for the non-LLVM compiler; crossing it is a release
  blocker.
- Do not add anything to the core that could live in a plugin/library if it would push the
  core past budget — but **do not** drop a capability to hit the number; move it out
  instead.
- Moving machinery out of the core must not change observable language semantics or
  correctness.
- **No regression from new features.** Every approved feature — generic impls, the borrow
  checker, a new backend — must be implemented for speed and must **not** regress compile
  time, peak memory, or generated-code quality. Shared machinery belongs in **MIR**, and
  adding a backend must stay **thin** (no duplicated front-end/dispatch logic).
- "Small" is measured on the **release** artifact (not a debug build), and size, compile
  time, peak memory, and generated-code quality are measured together — see
  [`mir-migration-evaluation.md`](mir-migration-evaluation.md) and the MIR baseline
  contract in [`mir-implementation-plan.md`](mir-implementation-plan.md) §11.

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

### 6.7 Native GC mode — one syntax, GC-managed, still a native binary

We need a mode where the user/agent can **stop caring about allocation and free**, and
the code still produces a **native executable**. Not a hosted runtime: the collector is
part of the produced binary (Go-style), not a JVM/browser/WASM/interpreter. This is a
first-class goal of Requirement 3, not a workaround.

> **Status (2026-10-13): approved** — see
> [`proposals/0006-native-gc-mode.md`](proposals/0006-native-gc-mode.md). The model:
> **explicit per-module opt-in** (`gc = true`); a GC module may depend only on GC modules,
> and a GC module in a non-GC build errors (*module requires gc, no gc on the
> job/executable/program*), so GC cannot silently enter GC-free code. **Collector:**
> conservative (Boehm-style) first. **Borrow checking is kept** for aliasing/races
> (lifetimes relaxed); **`@delete` becomes an optional finalizer**. It gates the JVM and
> Android targets.

**Intent**

- A program can opt into a **GC memory model** in which heap values are reclaimed
  automatically; the user never tracks allocation or free for them.
- The output is a native executable from the **C and LLVM backends** with a collector
  **statically linked in** (like Go's runtime is embedded in the binary). No VM in the
  loop; the process runs on the metal.
- **Opt-in only, zero tax otherwise.** The existing owned / deterministic-`@delete`
  model stays the default. GC mode must not impose a runtime cost, a larger binary, or a
  semantic change on code that does not use it (Requirement 0, §7 tension 1). Making GC
  the implicit default would violate the performance pillar.

**Why we want it**

- It is a second, VM-independent "runs everywhere" axis: one can ship a native binary
  with automatic memory management. It complements the VM targets (JVM/browser) rather
  than replacing them.
- It is the honest escape hatch from borrow-checker friction for **graph-shaped,
  reference-heavy code** (DOM-like trees, caches, interpreters, ASTs) without sending
  that code to a GC VM.
- For agents: many correct programs do not need deterministic destruction. Choosing GC
  removes a class of lifetime bookkeeping, and the agent no longer has to get ownership
  right for that code.

**The central question: does the language semantics change for this mode?**

This is **unsolved**. The candidates we must evaluate (none decided):

1. **One syntax, two memory models.** Owned types keep move/borrow/deterministic
   `@delete`; GC types are freely aliasable and reclaimed by the collector. The open
   part is the *granularity* of the choice and the boundary rule:
   - *whole-program / per-build target* (simplest to implement; coarse), or
   - *per-module* (a module declares its memory model), or
   - *per-type* (a `@gc` marker; fine-grained, but needs a containment/crossing rule).
2. **What happens to destructors.** `@delete` is deterministic and synchronous; a GC
   finalizer is best-effort and may never run. RAII is the correct tool for **OS
   resources** (files, sockets, locks) and must stay deterministic even in GC mode.
   So GC types either lose destructors, or destructors degrade to finalizers that must
   not be relied on for resource cleanup (resource types would need an explicit
   `close`/scope mechanism).
3. **Is the borrow checker relaxed, disabled, or kept for GC values?** The collector
   guarantees **memory** safety on its own, so lifetimes are not needed for that. But
   if aliasing is relaxed to "shared mutable", we give up compile-time
   **data-race / aliasing** guarantees for those values. Options: keep ownership but
   still GC-free the storage (buys little), relax aliasing and accept Go-like
   data-race exposure, or relax aliasing but require a shared-mutable type to be
   explicitly marked (so races stay opt-in). This is the biggest semantic fork.
4. **Crossing the boundary.** Can a GC object reference an owned value (a pointer into
   a non-managed allocation), and vice versa? Every native-GC language has to answer
   this. Candidate rule: *GC types may only reference GC types* (a containment
   discipline, cf. Swift/ARC and C# GC handles), with explicit rooting/pinning for the
   rare crossing. Without a rule, an owned value referenced from a GC object can dangle
   the moment its owner leaves scope.
5. **Shared "managed reference" with JVM/JS.** The JVM and JS targets are already
   GC-managed. If native-GC, JVM, and JS share one *managed reference* concept, the same
   source could target native-GC and JVM with analogous semantics — a large unification
   win that also de-risks §6.5/§6.6.

**Implementation (candidate directions, not decisions)**

- **Route allocation and roots through MIR.** MIR is now the single lowering path. GC
  mode can (a) route `Alloca`/heap allocation of GC types to the collector, (b) replace
  `Drop`/`Destroy` for GC types with finalizer registration or nothing, and (c) record
  **safepoints** and **root descriptions** as MIR metadata, so every backend gets them
  for free. This is why the "one shared core + thin backends" work (see
  [`mir-migration-evaluation.md`](mir-migration-evaluation.md)) is a prerequisite:
  four backends each re-deriving memory layout would make a correct GC impossible.
- **Root discovery.** Two viable strategies, in increasing order of cost/quality:
  1. **Conservative stack scanning** (Boehm-style): the collector scans the stack and
     registers for anything that looks like a pointer. Small compiler change, works
     with the existing C/LLVM output, but cannot move objects and can retain garbage by
     accident.
  2. **Precise stack maps / shadow stack** emitted by the backend: enables
     **moving/compacting** and concurrent collectors and better performance. LLVM has
     first-class support to build on (`gc.statepoint` / `llvm.experimental.gc.*`, the
     `gcroot` intrinsics, pluggable GC strategies such as `statepoint-example`), so we
     do not need to invent a backend from scratch.
- **The collector runtime.** A small collector library linked into the executable,
  selected per target. It may eventually be written in Chemical itself (we already have
  `unsafe`, raw pointers, atomics, and C interop), but vendoring a proven collector
  (e.g. a Boehm-style C library) is the realistic first step. Low-pause concurrent
  collection is achievable natively (Go demonstrates it), so pause behavior is a design
  constraint, not a blocker.
- **FFI.** The C ABI boundary is where a moving collector is hardest; a non-moving or
  pinned region at the boundary is the pragmatic first answer.

**What we explicitly do not know yet**

- Whether semantics change at all, and if so the boundary rule (whole-program vs
  per-module vs per-type).
- Whether the borrow checker is relaxed or kept for GC values, and what guarantee
  replaces it for **data races** (memory safety is provided by the collector).
- The concrete collector (conservative vs precise, moving vs non-moving, concurrent vs
  stop-the-world) and its interaction with `unsafe`, FFI, and the C ABI.
- How GC and owned values interoperate (containment, rooting, pinning) and what happens
  at the FFI boundary.
- Whether there is one managed-reference concept shared across native-GC, JVM, and JS.

**Non-negotiable constraints for any design here**

- GC must be **opt-in** and must not tax owned code (Requirement 0).
- GC mode must still produce a **native executable**; a design that requires a hosted
  VM is out of scope for this mode.
- Deterministic resource cleanup (files/sockets/locks) must remain available and must
  not depend on GC finalization.
- Memory safety remains a hard guarantee (Requirement 1); the collector must not trade
  it away, and `unsafe` stays explicit and scoped.

> **Target matrix (updated):** native-owned (C + LLVM), **native-GC (C + LLVM)**,
> **WASM**, **JS/browser**, **JVM bytecode**, **Android**.

### 6.8 Generic implementations are not supported

Chemical can implement an interface for a **concrete** type, and it can implement one
for a **generic type only as a *nested* impl** inside that type's body (which is
instantiated along with the containing type). It **cannot** declare a *parameterized*
implementation — one that is general over the impl's own type parameters — anywhere
else. Today:

- `impl <T> I for S<T>` is **rejected** (`don't declare generic parameters`); the
  compiler only uses the parameters of the implemented type.
- A parameterized impl whose target is a generic instantiation does not resolve its
  methods in general: `impl Future<T> for FutureHandle<T>` fails with
  `unresolved child 'poll'` (verified; the async code has to recognise
  `FutureHandle<T>` **structurally** and build vtables by hand to work around it — see
  `async-await-design.md`).
- The only working form is a **nested** impl inside the generic type's body — e.g.
  `struct S<T> { ..., impl I for S<T> { ... } }` — which is instantiated along with the
  containing type (positive test `impl_inside_generic_struct_allowed`).

In other words, **there is no way to say "for every `T`, `MyType<T>` satisfies
`Trait`" outside the type declaration itself.** Each `impl` binds to one concrete
instantiation, so generic types cannot participate uniformly in protocols.

**Why it matters**

- Most Rust-ecosystem and standard-library patterns are parameterized impls:
  `impl<T> Iterator for MyIter<T>`, `impl<T> Ord for MyType<T>`,
  `impl<U> From<U> for MyType<U>`, `impl<T> Serialize for Wrapper<T>`. Without them,
  generic types cannot be abstracted over, so libraries must duplicate code per
  instantiation and the "runs everywhere" library ecosystem is much smaller.
- It blocks trait-based abstractions (iteration, conversion, operators, serialization,
  formatting) for any type that is itself generic.

**Open questions (unsolved)**

- Whether to support them by **monomorphizing the impl** the same way generic
  functions/structs are already instantiated (the generic instantiation pass exists),
  versus dictionary/vtable dispatch.
- How to handle **bounds / where-clauses** on impl parameters (`impl<T: Ord> ...`) and
  how they interact with the no-panic/effect policy.
- How parameterized impls interact with the implementations index, generic method
  resolution, and the interface vtable scheme (the async code's structural workaround is
  a symptom of the missing mechanism).
- Whether the `impl<T>` header should be accepted as sugar when `T` is inferable from
  the target type, rather than rejected as it is today.

**Direction:** make generic impls monomorphize like generic functions, so the same
syntax covers concrete and parameterized targets. Until then, the only supported form is
a nested `impl` inside the generic type, and library authors must instantiate per type.

> **Status (2026-10-13):** the direction is **approved** — proposal
> [`0003-generic-impls`](proposals/0003-generic-impls.md). It is a prerequisite for
> proposal [`0014-generic-container-serialization`](proposals/0014-generic-container-serialization.md)
> (JSON for `vector`/`unordered_map`/`ordered_map`). When implemented, the async
> `FutureHandle<T>` structural workaround can be retired.

### 6.9 Roadmap status (2026-10-13)

Decisions from the proposal reviews (full log in
[`proposals/DECISIONS.md`](proposals/DECISIONS.md)):

- **Approved, to implement after the MIR consolidation work:** generic impls (`0003`),
  borrow checker / memory-safety closure (`0004`), no-panic effect system (`0005`), native GC
  mode (`0006`), JavaScript backend (`0007`), web/DOM type declarations (`0008`), async
  completion (`0013`), generic container JSON serialization (`0014`).
- **In progress, no proposal:** MIR core consolidation (`0001`) — see
  [`mir-migration-evaluation.md`](mir-migration-evaluation.md).
- **Effect system (decided):** square-bracket specifiers; v1 effects are `panic`, `pure`,
  `blocking`, `unsafe`, `alloc`, `noreturn`; restrictions are **per-function**
  (`[no_panic]`, `[no_alloc]`, …, `[pure]` = forbid all) with an optional module default;
  effects are **declared, never inferred** across current-module forward calls (error
  instead); and `unreachable`/traps are **`unsafe`**, not `panic` — so
  `[no_panic, no_unsafe]` is what truly prevents an abort. See
  [`proposals/0005-effect-system.md`](proposals/0005-effect-system.md).
- **Native GC gates the managed targets:** JVM (`0010`) is reconsidered now that GC is
  approved; Android/cross-compilation (`0011`) stays deferred until the GC model works.
- **WASM (`0009`) is deferred** — TinyCC is adding TLS and may add a WASM backend; revisit
  when a concrete need appears.
- **No standalone stdlib proposal** for now (`0012`); only a shared IO abstraction
  (`Reader`/`Stream`, `0015`) is a candidate.
- **Standing constraints:** every approved feature must be implemented **without regressing
  compile time, peak memory, or generated-code quality** (shared machinery in MIR, thin
  backends); and a large body of **common core-language tests** must run on every execution
  path — interpreter, TCC, LLVM (and future backends) — after each feature.

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
6. **Deterministic destruction vs. GC convenience** — one syntax must serve both the
   owned/`@delete` model and an opt-in GC model on the same native backend (§6.7),
   without letting the GC model's relaxed guarantees leak into owned code, and without
   making GC the default.
7. **Small core vs. full capability** — keep the compiler small by moving non-essential
   machinery into plugins/libraries, **never** by dropping language features or backends.
   Size, compile time, and memory efficiency are improved together, under the 5 MB
   ceiling (goal 4 MB), over the two-phase release plan (§2 toolchain constraint).

---

## 8. Non-goals

- We are **not** building another Rust, and will not copy Rust syntax wholesale.
- We are **not** optimizing the language for human authoring convenience.
- We are **not** adding type inference / sugar that costs compile time for little
  verification value.
- We are **not** accepting memory-unsafe escape hatches as the normal path (they remain
  explicit, scoped, and opt-in — see the existing `unsafe` marker).
- We are **not** making a garbage collector the default or the only memory model, and we
  will **not** pay a GC/runtime tax on code that opts out of it (Requirement 0).
- We are **not** requiring a hosted VM (JVM, browser, WASM, interpreter) to get GC; the
  native GC mode (§6.7) must produce a real native executable.
- We are **not** shipping a "lite" compiler: the small non-LLVM compiler supports the
  whole language and all targets it owns. Weight is moved to plugins/libraries, not
  capabilities.
- We are **not** accepting a permanently large compiler: a large first release (7–15 MB)
  is a starting point only, after which size, compile time, and memory are reduced
  together until the non-LLVM compiler is under the 5 MB ceiling (goal 4 MB).

---

## 9. How to apply this document

When working on the language, compiler, libraries, or toolchain:

1. **Check the requirements** (§2) — does the change move us toward, or away from,
   performance / memory safety / no-panic enforcement / running everywhere? Also check the
   **toolchain constraint** (§2): does it grow the core compiler past the 5 MB ceiling, or
   slow it / bloat its memory? If so, move the machinery to a plugin/library instead of
   dropping the capability.
2. **Prefer constraints over sugar** (§4) — make wrong programs harder to write.
3. **Keep the Rust semantic familiarity** (§3) for memory safety, but keep Chemical
   syntax.
4. **Keep the human in the loop** (§5) — readable enough to review.
5. **Treat the blockers in §6 as the current critical path** toward Requirement 3.
6. **Measure on the release artifact** — binary size, compile time, peak memory, and
   generated-code quality are validated together (§2 toolchain constraint), never one at
   the expense of the others.

If a design decision needs to trade one pillar against another, document the trade
explicitly in the relevant `lang/docs/` document so future agents inherit the reasoning.
