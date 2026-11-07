# Chemical Safety Sharpen Plan

> **Plan version:** 1.0  
> **Date:** 2026-11-07  
> **Status:** Phase 1 executed (2026-11-07); Phase 2 not started. See
> `lang/docs/safety-sharpen/STATUS.md` for the live job/evidence table.
> **Scope:** remove low-value surface added during the safety wave, close the
> safety gaps that prevent safe APIs, reshape every shipped library around safe
> APIs, and set a release gate for a safe-API release before implementation
> hardening continues.  
> **Not in scope:** new syntactic sugar, new backends, GC implementation, JVM/JS
> implementation, and feature-parity work that is not required to express safe
> code.

---

## 0. Executive summary

> **Phase 1 result:** completed in commit `safety-sharpen: complete phase 1
> removals and release sizing`; release `TCCCompiler` is **4,037,816 bytes**
> (under the 5 MB hard ceiling). The remaining Phase 1 exception is the
> pre-existing TCC `--server` C-emitter failure recorded in
> `lang/docs/safety-sharpen/BLOCKED.md`; it is Phase 2 codegen work.


Chemical now has a real safety model: MIR-owned move/init/drop state, borrow and
lifetime checking, `Send`/`Sync`, unsafe/FFI boundaries, target-aware C ABI,
`Copy`/`Zeroable`, async frame lifetime checks, and a large safety test suite.
That work is the foundation and must not be weakened.

The safety wave also did two things the project did not ask for:

1. It expanded the language surface toward Rust parity (associated
   types/projections/equality constraints, nested pattern forms, `Box<[T]>`,
   external/generic inherent impls, and several convenience forms). Most of
   these are unused in shipped libraries, add parser/type-check/
   generic-instantiation/MIR paths, and increase compile time and compiler size.
   This plan keeps the features that are genuinely useful or pre-existing
   (`dyn`/`DynBox` for dynamic dispatch, tuple structs, unit tuple, core tuples,
   flat destructuring, slices/`str`), and removes only the low-value additions.
2. It checked safety in the compiler but did not finish making the *shipped
   libraries* safe. The audit baseline still reports `3828` unsafe blocks,
   `4971` unsafe values, `9416` raw-pointer types, and `1085` raw-pointer
   dereferences. Much of that is necessary FFI, but a large part is public API
   shape: raw pointers, out-parameters, `*char` paths, `*mut` handles, and
   `unsafe func` entry points that force users to write unsafe code for ordinary
   work.

The result is a language that can prove more than its libraries let users write
safely. The compiler is also over the 5 MB non-LLVM ceiling (last recorded
release binary: **5,196,920 bytes**), and the current release build does not
compile because `GenericInstantiator::restoreRequirement` uses an optional in
non-`DEBUG` builds (`compiler/generics/GenericInstantiator.h:177-183`).

### What this plan changes

**Keep** the safety model and the features that express it:

- ownership, moves, deterministic `@delete`, partial initialization, drop flags;
- borrow/lifetime checking, NLL loans, HRTB call regions;
- `Copy`, `Clone`, `Default`, `Send`, `Sync`, `Unpin`, `PhantomData`;
- slices/`span`/`mut_span`/`str` as safe borrowed views with explicit lifetimes;
- tuples as a core value type (the user explicitly wants `pair._0`);
- tuple structs and the unit tuple (retained user-facing value types);
- `dyn` trait objects, dynamic calls, object safety, and `DynBox` (required for
  dynamic dispatch and plugin/runtime polymorphism);
- generic impls, basic `where` clauses, interfaces, auto traits;
- `Pin`, `Waker`, async frame lifetimes;
- `unsafe`, `@extern`, C ABI classification, `@repr`, packed/volatile/union rules;
- no-panic effects (`[no_panic]`) and the report → migrate → enforce workflow.

**Remove** feature families that are unused or low-value and cost compiler
complexity:

- associated types, associated-type projections, associated-type bounds and
  equality constraints (`T: Trait<Item = U>`);
- external/generic inherent `impl Type { ... }` blocks (methods in the type
  body and trait impls remain);
- nested tuple patterns, tuple patterns in `if` conditions, nested fixed-array
  patterns, slice condition/statement patterns, and only the slice/array-specific
  divergent-`else` lowering (variant/`Option`/`Result` `else` patterns stay);
- `Box<[T]>` and `std::boxed_slice<T>` (use `vector<T>` or a `[T]` view);
- range-slicing syntax (`view[a..b]`); keep method forms such as
  `view.subview(a, b)`.

Keep `dyn`/`DynBox`, tuple structs, and the unit tuple. They are useful,
pre-existing or explicitly requested, and their cost is acceptable compared with
the value they provide. The plan's job for them is to harden their safe surface,
not remove them.

Keep **flat** tuple destructuring and **flat** fixed-array destructuring because
they are used by real code (for example the playground's `useState` hook
destructuring) and they let users move elements safely without unsafe indexing.
Do not add more pattern shapes.

**Reshape** library APIs in one breaking wave:

- public user-facing types own their state through private fields;
- public safe APIs do not take or return raw pointers, `*char` out-buffers, or
  `*mut` handles;
- fallible operations return `Result`/`Option`; allocation has `try_*` forms;
- I/O uses `Read`/`Write` traits over `&[u8]`/`&mut [u8]`;
- paths use a typed `Path`/`PathBuf` and `OsString`;
- callbacks use `Fn`/`FnMut`/`FnOnce` bounds for static dispatch and a safe
  owning function wrapper for stored callbacks;
- raw FFI lives in explicitly named `raw`/`ffi`/`sys` modules and is the only
  place public raw pointers are allowed;
- every `unsafe` block carries a `// SAFETY:` justification and is covered by a
  safe API contract test.

### Release strategy

Three phases, not ten:

| Phase | Breaking? | Outcome |
|---|---|---|
| **Phase 1 — Baseline & sharpen** | Yes | Release build repaired; removed feature families deleted; compiler/store size measured; docs/skills reconciled; no new syntax. |
| **Phase 2 — Safe APIs** | Yes | Every shipped user-facing library has safe APIs, tests, docs, and audit gates. **Ship the safe-API release at the end of this phase.** |
| **Phase 3 — Harden & shrink** | Mostly no API break | Remaining safety parity gaps closed; implementation hardened; compiler size/compile-time driven toward 4–5 MB; no new sugar. |

Phase 2 is the release gate. After it, APIs are frozen for the release train;
Phase 3 improves implementations and safety internals without reshaping the
public API (except for genuine soundness fixes, which are documented as
breaking).

---

## 1. Analysis

### 1.1 What the safety wave built that must be kept

The enforced core is real and tested. Keep it:

- **MIR is the single owner of move/init/drop facts.** `MIRMoveCheck` catches
  uninitialized reads, double moves, use-after-move, partial moves, and
  destructor-bearing field/subobject moves. Per-field and per-element drop
  flags prevent leaks and double drops.
- **Borrow checker and regions.** `MIRBorrowCheck` enforces overlapping loans,
  moves/drops under borrow, return-region rules, storage lifetimes, temporary
  extension, variance, `PhantomData`, `Pin`, Waker ownership, async frame
  loans, and HRTB call regions.
- **Unsafe/FFI boundaries.** `SAFE-UNSAFE-*`, `SAFE-PROV-*`, `SAFE-FFI-*`,
  `SAFE-GLOBAL-*`, `SAFE-TRAIT-*`, `SAFE-VIS-*`, packed/union/volatile rules.
- **`Send`/`Sync` and data-race primitives.** Auto traits, explicit/negative
  impls, conditional generic impls, `Arc`/`Rc`/`Mutex`/`RwLock`/`Cell`/
  `RefCell`, scoped threads, TSAN coverage.
- **Validity/runtime checks.** Bounds, integer overflow/division/shift, float→int
  definition, allocation/layout, null, `zeroed`/`Zeroable`, enum tags,
  `@repr("transparent")`.
- **No-panic effects (first slice).** `[no_panic]` parsing, MIR symbol facts, and
  rejection of unannotated panics, indirect calls, dynamic dispatch, throws,
  and generated runtime panics.
- **Safety test infrastructure.** `--safety` suite, negative diagnostics,
  `SAFE-*` codes, runtime failure harness, audit gate, TSAN, `MIRTests`.

These are not negotiable and should not be simplified by removing checks. They
may be restructured for compile-time/size wins, but the enforcement must stay.

### 1.2 What was added beyond the safety requirement

The following feature families were added during the safety wave. Some are
retained because they are pre-existing, explicitly requested, or needed for
dynamic dispatch; the rest are low-use and proposed for removal.

| Feature family | Where it landed | Usage evidence | Cost | Disposition |
|---|---|---|---|---|
| `dyn` trait objects / `DynBox` | `ast/types/DynamicType.*`, `MIRBorrowCheck`, `AutoTraits`, `TypeVerify`, `std::DynBox` | Dynamic calls are required for runtime/plugin polymorphism; `DynBox` is the owning form | ~900 net lines; object-safety/upcasting/dyn bounds; MIR vtable/borrow paths | **Keep** |
| Associated types | `Typealias` declaration/binding, `GenericInstantiator`, `LinkSignature`, `TypeVerify` | 0 shipped-library declarations; 3 interface tests; 1 probe | ~1,200 net lines; projection/equality paths in generic instantiation | **Remove** |
| Tuple structs / unit tuple | `StructDefinition`, parser, generic instantiation | 4 tuple-struct tests; unit tuple in tests; user explicitly wants them | ~300 net lines; positional constructors, field naming, pattern lowering | **Keep** |
| Nested/condition patterns | `PatternMatchExpr`, `MIRLowerCall`, `SymResLinkBody`, `VarInitialization`, `IfBlock` | Flat array destructuring used in playground; nested/slice forms only probes/tests | ~1,400 net lines; runtime length checks, divergent-else lowering | **Remove sugar; keep flat forms** |
| Slices/`&str` aliases | parser aliases, `std::span`/`mut_span`/`str`, `MIRBorrowCheck`, `TypeVerify` | `std::span` used heavily; `&str` only a few places; `Box<[T]>` only docs/tests | ~1,500 net lines incl. library; view syntax itself is cheap | **Keep views; remove `Box<[T]>`** |
| External/generic inherent impls | `GenericImplDecl`, `LinkSignature`, `SymResLinkBody`, mangler | 0 shipped-library uses; 1 test | ~100 net lines; duplicate resolution paths | **Remove** |
| Range slicing, windows/chunks/split | parser/MIR + `std::span` library | mostly docs/tests; methods are safe and useful | parser/MIR syntax cost; library code is cheap | **Keep methods; remove range syntax** |

The problem is not that these features are individually enormous. The problem
is that each one adds a parser path, a type-check path, a symres/generic path, a
MIR lowering path, a codegen path, a test surface, a docs surface, and a skill
surface. Together they make the compiler larger and slower and make the language
harder for an agent to reason about than it needs to be.

### 1.3 Current safety gaps that block safe APIs

These are verified gaps from the safety plan/skill, plus gaps found while
auditing the libraries for this plan.

1. **No-panic is incomplete.** Only `[no_panic]` exists. `[no_alloc]`,
   `[no_unsafe]`, `[no_blocking]`, `[pure]`, module defaults, and
   generic/async/interface propagation are missing. Most std operations panic on
   allocation failure, bounds, `Option::take` on `None`, etc., so a
   `[no_panic]` module cannot use most of `std`.
2. **Allocation failure is not a first-class API surface.** `vector`, `string`,
   `deque`, `Box`, `Rc`, `Arc`, thread pool, and async constructors mostly panic
   or abort on OOM. `std::alloc` already returns `Result<_, AllocError>`, but the
   containers do not expose `try_*` forms.
3. **Public API shape forces unsafe.** `fs`, `path`, `process`, `net`, `http`,
   `tls`, `archive`, `image`, `audio`, `font`, `window`, and `webview` expose
   raw pointers, out-buffers, `*char`, `*mut` handles, or `unsafe func` entry
   points for ordinary work.
4. **Callback lifetime/ownership is not fully modeled.** `std::function` is
   safe, and `SAFE-LIFE-001` catches returning a borrowed capture, but there is
   no single safe stored-callback type that proves `'static` ownership for
   long-lived registrations (window/webview callbacks, thread pools, global
   tables). The current workaround surface (`std::function`, `send_function`,
   `scoped_send_function`, `@scoped_callback`, `%capture` witnesses) is larger
   than it needs to be.
5. **Scoped threads are over-specialized.** `std::thread` has many scoped
   variants, erased variants, witness types, and `@scoped_callback` body checks.
   A `thread::scope`-style API with a scoped `Scope` type and `Fn` bounds would
   express the same safety with less compiler special-casing.
6. **`Pin` is not usable safely.** `Pin::new`/`Box::pin`/`Pin::get_mut`/`into_inner`
   are missing or `unsafe`-only; users must use `pin_unchecked`.
7. **`FutureHandle` and callback custom-consumer analysis is conservative and
   complex.** The `SAFE-LIFE-005` consumer whitelist and HRTB source-to-stored
   inference are large special cases. Some are sound; some can be replaced by
   simpler safe API shapes.
8. **FFI/C ABI is close but not closed.** Unknown-target by-value aggregates,
   variadic aggregate arguments, callback ABI audits, and some `@stdcall`
   shapes remain. `cstd` is raw by design, but library wrappers must validate
   lengths, nulls, alignment, UTF, and ownership.
9. **Send/Sync conservatism remains** for executor/iterator raw state,
   elided/named-lifetime views, erased `FutureHandle`, and unbound `dyn` (which
   stays; the conservative `!Send`/`!Sync` rule is documented as a safe
   boundary, not a removal target).
10. **The safety docs are too large and partly stale.** The safety plan is
    ~3390 lines with a historical log and duplicated status; `AGENTS.md` still
    says generic impls are unsupported and carries a 2026-11-01 status; feature
    docs describe associated types as permanent even though this plan removes
    them. This is an agent-context problem, not just a docs problem.

### 1.4 Compiler size and release blockers

- Last recorded release `TCCCompiler`: **5,196,920 bytes**, over the 5 MB
  hard ceiling (goal 4 MB). The language-goals document allows a much larger
  *first* release (7–15 MB) as a temporary state; this plan treats the safe-API
  release as the steady-state artifact and therefore enforces the 5 MB ceiling
  at the Phase 2 gate. A temporary first-release exception requires the user's
  explicit approval and a dated reduction plan.
- Current release build fails:

```
compiler/generics/GenericInstantiator.h:181:
  inline void restoreRequirement(std::optional<InstantiationRequirement> req) {
      _requirement = req;   // error in non-DEBUG builds
  }
```

  In `DEBUG`, `_requirement` is `std::optional<...>`; in release it is
  `InstantiationRequirement`. `saveRequirement` itself converts fine; only
  `restoreRequirement` assigns the optional back into the non-optional field.
  Guard both for consistency. This must be fixed before any size measurement or
  release work.
- Debug `TCCCompiler` is ~94 MB; `Compiler` (LLVM) is ~337 MB. These are not
  release artifacts, but they show how much code is in the compiler.
- `compiler/mir/MIRLowerCall.cpp` is ~11k lines, with many array-shape-specific
  drop selectors (1D/2D/3D/4D, small/large/mixed, generated-glue templates,
  runtime trees). This is the largest single safety complexity hotspot.
- Feature code and safety code both contribute; the plan measures both.

### 1.5 Library unsafe inventory

The whole-tree audit is conservative but useful. Current totals from
`lang/docs/safety/audit-baseline.json` / skill:

| Metric | Count |
|---|---:|
| `unsafe_blocks` | 3828 |
| `unsafe_values` | 4971 |
| `raw_pointer_types` | 9416 |
| `raw_pointer_derefs` | 1085 |
| `raw_pointer_addrs` | 10050 |
| `raw_pointer_casts` | 2900 |
| `unsafe_func_decls` | 52 |
| `extern_calls` | 1322 |
| `panic_calls` | 142 |

Largest module surfaces (audit): `css_parser` 462 unsafe blocks, `tls` 375,
`universal_cbi` 228, `std` 183, `window` 123, `process` 104, `html_cbi` 91,
`webview` 82, `page` 77, `async` 73, `fs` 64, `http` 60, `image` 50.

Not all of that is debt. `cstd`, parser/CBI internals, cryptographic primitives,
TLS record processing, and windowing bindings are inherently FFI-shaped. The
debt is **public API shape**:

| Library | Public raw-pointer funcs* | Public `unsafe func`s | Unsafe blocks | Raw-pointer types |
|---|---:|---:|---:|---:|
| `cstd` | 348 | 8 | 10 | 3983 |
| `tls` | 137 | 4 | 377 | 935 |
| `window` | 78 | 0 | 194 | 416 |
| `std` | 70 | 22 | 275 | 750 |
| `webview` | 50 | 0 | 165 | 804 |
| `fs` | 48 | 0 | 216 | 210 |
| `archive` | 41 | 0 | 47 | 134 |
| `async` | 35 | 9 | 147 | 179 |
| `image` | 33 | 0 | 62 | 130 |
| `compiler_runtime` | 24 | 0 | 22 | 135 |
| `lab` | 24 | 2 | 4 | 96 |
| `crypto` | 22 | 0 | 44 | 60 |
| `net` | 16 | 2 | 53 | 147 |
| `http` | 15 | 4 | 79 | 133 |
| `process` | 13 | 0 | 105 | 70 |
| `core` | 12 | 11 | 8 | 50 |
| `path` | 8 | 2 | 13 | 17 |
| `json` | 5 | 0 | 95 | 114 |
| `page` | 5 | 0 | 81 | 114 |
| `regex` | 2 | 0 | 23 | 37 |

\*Counts are source-text approximations from `public ... func ... *`; they are a
triage aid, not an exact API inventory. The execution phase replaces this with a
generated public-API snapshot.

Measured goals for the safe-API release (not estimates):

- Tier A public raw-pointer/`unsafe func` surface: **zero** outside Tier C plus
  the reviewed explicit allowlist.
- Tier A user-facing modules' `unsafe_blocks`: **at least 50% below** the
  current baseline, with the remainder concentrated in named `raw`/`ffi`/`sys`
  modules. Moving code into a raw module without a safe wrapper does not count
  as a reduction.
- Tier A `panic_calls`: only documented panicking conveniences (`Index`,
  `unwrap`-style, allocation `new`/`push`); the fallible surface has no
  reachable panic. Measure with `--safety-report` and a no-panic sample.
- Every Tier A library has a `SAFETY.md` boundary map and contract tests.

The user-facing problem is concrete:

- `path::basename(*char, out: *mut char, out_len)` forces raw pointers for a
  pure string operation.
- `fs::read_entire_file(*char)` forces a C string for a path.
- `process::wait(*mut ChildProcess)` forces an unsafe pointer for every child
  operation.
- `net::Socket` is a bare `usize`; `recv_all`/`send_all` take raw buffers.
- `http::Body.read(*mut u8, cap)` and `parse_request_from_bytes(*u8, n, ...)`
  force unsafe buffer handling.
- `tls` exposes mbedTLS-style raw contexts for normal client/server work.
- `archive`/`image`/`audio`/`font` expose `*mut` handles and out-params.
- `window`/`webview` expose `*mut` handles and `*mut void` callback data.
- `std::vector::get_ptr`, `span::get` returning `*T`, `string` raw
  constructors, `Option::take` panicking on `None`, etc. push users toward
  unsafe or panicking code.
- `test_env` `info/warn/error` take `*char`.

### 1.6 Feature ranking method

Each feature is scored 0–5 on:

- **Usage (U):** shipped libraries + real compiled apps + tests. 0 = nothing,
  1 = tests/probes only, 3 = one real user, 5 = pervasive.
- **C/C-with-classes analogue (C):** does it exist in C or a small C-with-classes
  style language? 5 = core C, 0 = no analogue.
- **Safety value (S):** does it help prove memory/thread/init safety or express a
  safe API? 5 = required, 0 = no safety value.
- **Compiler cost (K):** parser/typecheck/symres/generic/MIR/codegen/test/docs
  complexity. 5 = very high, 0 = trivial.

Decision rule:

- Remove if `S <= 2` and `U <= 2` and `K >= 3`, unless it is a widely expected
  primitive that costs almost nothing.
- Keep if `S >= 4` and a safe API needs it.
- Keep if `C >= 4` and it is a core data/control primitive.
- Freeze (do not implement) if `S <= 3` and no concrete safe API needs it.
- Keep a small core form and remove the sugar around it when the sugar is the
  expensive part.

### 1.7 Feature disposition table

| Feature | U | C | S | K | Decision |
|---|---:|---:|---:|---:|---|
| Tuple type/literal/`._N` access | 1 | 2 | 3 | 2 | **Keep core** (user requirement) |
| Flat tuple destructuring | 1 | 1 | 3 | 2 | **Keep flat only** |
| Nested tuple patterns / tuple patterns in `if` | 1 | 0 | 1 | 3 | **Remove** |
| Tuple structs / unit tuple | 1 | 1 | 0 | 2 | **Keep** (user requirement; low cost) |
| External/generic inherent `impl Type {}` | 0 | 0 | 1 | 2 | **Remove** |
| Fixed-array flat exact/rest destructuring | 1 | 1 | 3 | 3 | **Keep flat only** |
| Nested fixed-array patterns | 1 | 0 | 1 | 4 | **Remove** |
| Slice condition/statement patterns + slice-specific divergent `else` | 1 | 0 | 1 | 4 | **Remove slice-specific lowering only** |
| Variant/`Option`/`Result` patterns + `else` | 5 | 1 | 4 | 2 | **Keep** |
| `[T]`/`&[T]`/`&mut [T]`/`&str` view syntax | 3 | 4 | 5 | 2 | **Keep** |
| `Box<[T]>` / `boxed_slice<T>` | 1 | 1 | 2 | 2 | **Remove** (use `vector<T>`) |
| Range slicing `view[a..b]` | 1 | 1 | 1 | 2 | **Remove syntax; keep `subview`** |
| `windows`/`chunks`/`split`/`split_by` methods | 1 | 1 | 2 | 0 (library) | **Keep as safe library methods** |
| `dyn` trait objects + `DynBox` + object safety | 2 | 0 | 4 | 5 | **Keep** (dynamic calls are required; `DynBox`/probes/tests) |
| Associated types/projections/bounds/equality | 1 | 0 | 2 | 5 | **Remove** (no concrete safe API needs it) |
| Generic impls `impl<T> Trait for T<U>` | 4 | 1 | 4 | 3 | **Keep** |
| Basic `where` clauses / generic bounds | 5 | 0 | 5 | 2 | **Keep** |
| `Copy`/`Clone`/`Default` opt-in | 5 | 2 | 5 | 2 | **Keep** |
| `Send`/`Sync`/`Unpin` auto traits | 5 | 0 | 5 | 3 | **Keep** |
| `PhantomData` | 3 | 0 | 4 | 2 | **Keep** |
| `Pin` / `Waker` / async protocol | 4 | 0 | 5 | 4 | **Keep; add safe `Pin` API** |
| `[no_panic]` effects | 3 | 0 | 5 | 3 | **Keep; complete** |
| `@scoped_callback` special case | 2 | 0 | 4 | 3 | **Replace with `thread::scope`; then remove** |
| `%capture` witness types / `send_function` / `scoped_send_function` | 2 | 0 | 4 | 3 | **Keep internal; simplify public API** |
| `std::function` type erasure | 4 | 0 | 4 | 3 | **Keep; prefer `Fn*` bounds for new APIs** |
| `unsafe` / `unsafe(expr)` / inline asm | 5 | 5 | 5 | 2 | **Keep** |
| `@repr` / packed / transparent / volatile / union rules | 3 | 5 | 5 | 2 | **Keep** |
| `zeroed`/`Zeroable`/`@allow_zeroed` | 4 | 4 | 5 | 2 | **Keep** |
| `for-in`, `using`, `loop` expression | 5 | 3 | 1 | 1 | **Keep (pre-existing)** |
| `defer` | 0 | 0 | 1 | 2 | **Do not add** |
| `impl Trait` / GATs / const generics / built-in derive | 0 | 0 | 1–2 | 4–5 | **Freeze** |
| `impl Trait for (T1, T2)` | 0 | 0 | 0 | 3 | **Do not add** |
| Match/switch-position patterns | 0 | 1 | 1 | 3 | **Freeze** |
| Raw unsized `[T]` | 0 | 3 | 3 | 4 | **Do not implement**; views are enough |
| Full UTF-8 API parity | 1 | 0 | 2 | 2 | **Freeze** |

### 1.8 Inconsistencies found in docs and skills

These must be fixed in Phase 1:

- `AGENTS.md` says generic impls are not supported and that
  `impl <T> ...` is rejected. The source tree supports generic impls
  (JSON containers, auto traits, etc.). Update the architecture/roadmap text.
- `AGENTS.md` says the safety status is 2026-11-01 and describes old gaps; the
  safety plan is at v5.80. Pick one current-status document and link, not two
  stale copies.
- The safety plan is a 3,390-line mix of current status, historical log, and
  detailed design. Split it into:
  - `lang/docs/safety/current-status.md` (short, current, agent-facing);
  - `lang/docs/safety/design.md` (the borrow/init/FFI design, kept);
  - `lang/docs/safety/history.md` (the old chronological log, frozen).
- Feature docs (`tuples.md`, `slices.md`, `associated-types.md`,
  `dyn-object-model.md`) must match the final disposition: keep tuples/slices/
  dyn, delete associated types. Phase 1 updates or deletes them.
- `refgen` and `docgen` keyword lists include `dyn`; keep it because `dyn` stays.
- The safety skill and plan should keep `dyn` as a supported safe-surface feature
  but add the new safe `DynBox`/dynamic-call API notes.
- `lang/goals` says feature-parity work is frozen, but the commits implemented
  it anyway. The plan should record the decision and stop the drift.

### 1.9 Performance and size baseline

Current test baseline (latest measured):

| Gate | Result |
|---|---|
| TCC main | `2489/2489` |
| LLVM main | `2487/2487` |
| Safety (TCC/LLVM) | `190/190` |
| Negative (TCC/LLVM) | `871/871` |
| Async (TCC/LLVM) | `83/83` |
| TCC libs | `736/736` |
| TCC plugins | `1305/1305` |
| TCC interpret | `1938/1938` |
| `MIRTests` | `4749` checks |
| TSAN smoke | pass |
| Audit | no growth |

Release artifact: last recorded `5,196,920` bytes; current release build fails.
The first action of Phase 1 is to repair and re-baseline it.

Measurement rules:

- release `TCCCompiler`, not debug;
- median of 5 runs on an idle machine;
- record SHA, dirty state, target triple, output mode, compiler size, peak RSS,
  compile wall time;
- measure a small module, `std`-heavy module, and `lang/tests` build;
- record `size` sections and `nm --size-sort` top symbols;
- store results in `lang/docs/safety-sharpen/metrics.json` (new file created in
  Phase 1).

---

## 2. Decisions

### 2.1 Keep (non-negotiable)

- The entire safety enforcement pipeline and all `SAFE-*` diagnostics.
- Ownership/move/init/drop and partial-initialization tracking.
- Borrow/lifetime checking and `Send`/`Sync`/`Unpin`.
- `Copy`/`Clone`/`Default`, `Zeroable`, `PhantomData`.
- Slices/`span`/`mut_span`/`str` views and explicit lifetimes.
- Core tuples (`(T1, T2)`, literals, `._N`, flat destructuring).
- Tuple structs and the unit tuple.
- `dyn` trait objects, dynamic calls, object safety, dyn upcasting, and
  `DynBox`.
- Flat fixed-array destructuring for owned local arrays.
- Generic impls and basic `where` clauses.
- `Pin`/`Waker`/async protocol, with a safe `Pin` facade.
- `unsafe`, `@extern`, target-aware C ABI, `@repr`, packed/volatile/union rules.
- `[no_panic]` and the full effect-system direction.
- `@delete`/RAII, `MaybeUninit`, `ManuallyDrop`, `NonNull`.
- `std::function` type erasure and `Fn`/`FnMut`/`FnOnce` callable kinds.

### 2.2 Remove (breaking; Phase 1)

Remove code, tests, docs, and parser paths for:

1. Associated type declarations, bindings, projections, bounds, equality
   constraints, and `Self::Item`/`T::Item`.
2. External inherent `impl Type {}` and generic inherent
   `impl<T> Type<T> {}` blocks. (Tuple structs and unit tuple stay.)
3. Nested tuple patterns, tuple patterns in `if`, nested fixed-array patterns,
   and slice condition/statement patterns. Remove only the **slice/array-specific
   divergent-`else` lowering**; ordinary variant/`Option`/`Result` patterns and
   their `else` branches stay (`PatternMatchExpr`, `PatternMatchExprNode`,
   `PatternMatchIdentifier`, `PatternElseExprKind`, `ValueKind::PatternMatchExpr`).
   `json_cbi`-generated `Ok`/`Err` patterns and `if(var Some(v) = x) ... else
   ...` are core and must keep working.
4. `Box<[T]>` and `std::boxed_slice<T>`.
5. Range-slicing syntax (`view[a..b]`, `[..b]`, `[a..]`, `[..]`). Usage is
   test-only; rewrite those tests to `view.subview(a, b)`. Keep `subview`/
   `split_at` methods.

Do **not** remove:

- `dyn`/`DynBox`, dynamic calls, or object safety;
- tuple structs or the unit tuple;
- flat tuple destructuring;
- flat fixed-array destructuring;
- `[T]`/`&[T]`/`&mut [T]`/`&str` view syntax;
- library methods `subview`, `split_at`, `windows`, `chunks`, `split`,
  `split_by`;
- generic impls, interfaces, auto traits, `Pin`, `PhantomData`, async.

### 2.3 Freeze (do not implement)

- GATs, const generics, built-in derive, `impl Trait`/RPITIT, default-method
  parameter projections, `impl Trait for (T1, T2)`, match/switch-position
  patterns, raw unsized `[T]`, and full UTF-8 API parity.
- Any new syntax whose primary benefit is typing convenience.
- Any feature that increases compile time or compiler size without a concrete
  safe API blocked on it.

### 2.4 Replace, do not deprecate

This release is allowed to break. Remove old APIs and update every in-tree user.
For removed syntax, emit an explicit error with a migration hint rather than a
generic parse failure. For old library APIs, do not keep aliases; the release
notes and `BREAKING.md` provide the mapping.

### 2.5 Decisions requiring sign-off before Phase 1

These are the only genuinely judgment-heavy removals. Recommended defaults are
shown; the user can veto or modify.

| Decision | Recommendation | Why |
|---|---|---|
| Keep core tuples? | **Keep** | User explicitly wants `value._0`; low cost; core value type. |
| Keep flat tuple destructuring? | **Keep** | Used like `var (a, b) = pair`; cheap; helps safe moves. |
| Keep flat fixed-array destructuring? | **Keep** | Playground `useState` uses `var [value, set] = ...`; enables safe element moves. |
| Remove nested tuple/array patterns and slice patterns? | **Remove** | No shipped-library use; high lowering cost; safe alternatives exist. |
| Keep `[T]`/`&[T]`/`&mut [T]`/`&str` syntax? | **Keep** | This is the safe view vocabulary for the new APIs; parser cost is low. |
| Remove `Box<[T]>`/`boxed_slice`? | **Remove** | `vector<T>` owns; `[T]`/`&[T]` views cover borrowing. |
| Keep `dyn`/`DynBox`/dynamic calls? | **Keep** | Dynamic dispatch is required for runtime/plugin polymorphism; `DynBox` is the owning form. |
| Remove associated types? | **Remove** | No shipped-library use; large generic-instantiation surface; generic parameters suffice. Reintroduce only with a concrete safe API proposal. |
| Keep tuple structs and unit tuple? | **Keep** | Explicit user decision; small cost; useful value types. |
| Remove external/generic inherent impls? | **Remove** | No shipped-library use; methods in the type body or trait impls suffice. |
| Replace `@scoped_callback`/witness types with `thread::scope`? | **Replace in Phase 2** | Fewer compiler special cases and a better safe API. |
| Keep compiler/plugin raw APIs? | **Only under `compiler::ffi`/`lab::raw`; safe façade elsewhere** | Plugin internals are inherently C-ABI-shaped, but plugin authors need safe wrappers. |
| Allow a minimal `Deref`/associated-type carve-out if generic `Pin`/views need it? | **Only with explicit sign-off; default is concrete `PinBox`/`PinRef`/`PinMut`** | Associated types are removed; a narrow carve-out is a safety-critical exception, not a reintroduction of the full system. |

---

## 3. Target language

### 3.1 Feature policy

From this plan forward:

1. No new **sugar** is added unless a concrete safe API cannot be expressed
   without it. Safety-critical lifetime/type work explicitly scheduled in this
   plan (lifetime-carrying `span`/`str`, concrete `Pin` forms, `CString`/`Path`)
   is allowed and is not sugar.
2. Every feature proposal must state: safety justification, existing usage,
   compile-time/size cost, and the safe API it unblocks.
3. A feature that primarily saves typing is rejected.
4. A feature that lowers safety (e.g., an implicit unchecked path) is rejected.
5. Rust parity is not a goal; Rust *safety semantics* are.

### 3.2 Safety features required for safe APIs

These are the only language/safety additions planned. They are root-cause
features, not sugar.

#### A. Complete the no-panic effect system

Implement the remaining effects from proposal `0005`:

- `[no_alloc]` — no heap allocation (direct or transitive).
- `[no_unsafe]` — no `unsafe` block/operation in the body or transitively.
- `[no_blocking]` — no blocking syscall/sleep/lock-wait.
- `[pure]` — no observable side effects (definition fixed by the proposal).
- module-level defaults (e.g., a module declares `[no_panic]` for every
  function; functions can opt out only with an explicit audited annotation).
- propagation through generic functions, async functions, interfaces, and
  function-pointer calls; a call through an unknown function pointer is treated
  as may-panic/may-alloc.
- `panic`/`abort`/`unreachable` policy: `unreachable` and traps are explicit
  unsafe traps, not panics; the effect checker must distinguish them.

Acceptance: the Phase 2 audited sample module using the explicitly annotated
fallible core API compiles with zero reachable panics; a negative suite proves
each propagation rule that is implemented. Full generic/async/interface
propagation and the complete no-panic library matrix are P3.1. The release gate
must publish a per-Tier-A compatibility matrix (`safe`, `panics`, `untested`) so
"APIs ready" does not overclaim no-panic coverage.

**Dynamic-dispatch boundary:** the current `[no_panic]` slice rejects dynamic
dispatch. A `[no_panic]` module therefore cannot use `dyn`/`DynBox` until the
effect model gains a non-panicking vtable path (or an explicit `[panic]`/unsafe
opt-in). Document this in the effect docs and keep it fail-closed; do not remove
`dyn` to hide the gap.

#### B. Fallible allocation and fallible std

- `AllocError` and `Result`-returning allocation already exist in `std::alloc`.
- Add `try_*` forms to `Box`, `Rc`, `Arc`, `vector`, `string`, `deque`,
  `unordered_map`, `ordered_map`, thread pool, and async constructors.
- `try_*` must be zero-cost on success and must not panic on failure.
- Panicking `new`/`push`/`reserve` remain as convenience, but `[no_panic]`
  cannot call them.
- Allocation failure tests use a failing allocator (or a test hook) and assert a
  clean `Err` path with no leaks.

#### C. Safe `Pin`

The current `Pin<T>` wraps a raw pointer with no lifetime and `pin(&mut T)`
erases the borrow. A generic `Pin<P>` needs `Deref` associated-type machinery
(which Phase 1 removes). Therefore implement **concrete** safe pin forms:

```chemical
public struct PinBox<T> { ... }          // owns Box<T>
public struct PinRef<'a, T> { ... }      // borrowed shared pinned value
public struct PinMut<'a, T> { ... }      // borrowed mutable pinned value

public func <T> pin_box(value : T) : Result<PinBox<T>, AllocError>
public func <'a, T> PinBox<T>::as_ref(&'a self) : PinRef<'a, T>
public func <'a, T> PinBox<T>::as_mut(&'a mut self) : PinMut<'a, T>
public func <T> PinBox<T>::into_inner(self) : T where T: Unpin
public func <'a, T> PinRef<'a, T>::get(&self) : &'a T
public func <'a, T> PinMut<'a, T>::get_mut(&mut self) : &mut T where T: Unpin
public unsafe func <'a, T> PinMut<'a, T>::get_unchecked_mut(&mut self) : &mut T
```

If a later safe API genuinely needs the generic `Pin<P>` form, add a minimal
`Deref` associated-type carve-out as a separately approved safety feature; do
not silently reintroduce the full associated-type system. Keep
`pin_unchecked`/`from_raw`/`get_unchecked_mut` as unsafe primitives for `!Unpin`
runtime code. Add `Pin` contract tests for move/borrow/async.

#### D. Scoped threads and scoped callbacks

Replace the annotation/witness maze with a lifetime-parametric `Scope`:

```chemical
public func <F, T> thread::spawn(f : F) : JoinHandle<T>
    where F : core::ops::FnOnce<() => T>, T : Send + 'static
    // plus the capture witness must be Send + 'static

public struct Scope<'scope> { ... }
public struct ScopedJoinHandle<'scope, T> { ... }

// Function-type parameter uses the supported for<'s> form.
public func thread::scope(body : for<'s> (&'s mut Scope<'s>) => void)

public func <'scope, F, T> Scope<'scope>::spawn(&mut self, f : F)
    : ScopedJoinHandle<'scope, T>
    where F : core::ops::FnOnce<() => T>, T : Send + 'scope
    // plus the capture witness must be Send; use the existing %capture/
    // send_function witness mechanism rather than assuming a bare F is Send
```

`Scope` internally owns a `ScopeData` with type-erased join records (a function
pointer + data pointer, or an enum/vtable record) and a join-on-drop list; the
typed `ScopedJoinHandle<'scope,T>` is a safe index/handle into it. The exact
type-erasure mechanism must be designed and tested before deleting
`@scoped_callback`. Captures must be `Send` (and borrowed values must satisfy
the scoped `Send`/`Sync` rules); the borrow checker checks them against
`'scope`. No `@scoped_callback` marker or `scoped_send_function` public type is
needed. If the current borrow checker cannot express the `for<'s>` form, the
implementation may use the equivalent supported lifetime-parametric form, but it
must remain a library API with no special compiler annotation. Fixing the
checker is a safety-critical language task, not a workaround.

#### E. Safe stored-callback type

For long-lived registrations (window/webview callbacks, thread pools, event
tables), do **not** invent a new arbitrary `F: FnMut(...) + Send` erasure
mechanism. The shipped model is:

- `std::function<T>` is built from a literal lambda through the compiler's
  capture intrinsics; it is the general type-erased callable.
- `std::typed_function<T, C>`, `std::send_function<T>`, and
  `std::scoped_send_function<T>` are the witness-carrying forms.
- The `Fn`/`FnMut`/`FnOnce` marker types are deliberately `!Send`/`!Sync`; do
  not claim `FnMut + Send` is derivable today.
- `default_function_instance` holds raw pointers and has no proven
  `Send`/`Sync`; do not claim `std::function` derives them from the capture
  until a witness rule is implemented. Cross-thread callbacks use
  `send_function`/`SendStaticCapture` (or a new proven witness), not
  `std::function` with an assumed bound.

Provide thin safe stored-callback wrappers that reuse those witnesses:

```chemical
// Single-threaded UI callback (may be !Send). Construction takes the existing
// %capture witness, not an arbitrary F.
public struct Callback<Args, R> {
    private var f : std::function<(Args) => R>
}

// Send + 'static callback for thread pools / cross-thread registration.
public struct SendCallback<Args, R> {
    private var f : std::send_function<(Args) => R>
}

public func Callback<Args, R>::call(&mut self, args : Args) : R
public func SendCallback<Args, R>::call(&mut self, args : Args) : R
```

Use `Callback<(), R>` for no-argument callbacks and `Callback<(A, B), R>` for a
two-value call; the tuple is passed as one argument, matching Chemical function
types. The wrapper owns the capture through the existing private allocation and
has a `@delete` that drops exactly once. Do not expose `*mut void` user data in
public APIs. Before promising these wrappers, add a Phase 2 spike/test proving
the exact construction syntax, `Send`/`Sync` witness, and drop behavior; if
`std::function` cannot wrap an arbitrary witness, keep the existing
`send_function`/`typed_function` forms as the public API and do not invent
`Callback`.

The `[no_panic]` example in Phase 2 must use the generic `F: Fn*` path or the
fallible non-callback APIs, not `Callback`/`std::function`, because type erasure
is a dynamic-dispatch boundary until the effect system models a non-panicking
vtable path. This is a scoped example restriction, not a ban on callbacks.

#### F. C string / OS string / path primitives

Safe FFI and I/O need owned, validated string types:

- `std::CString` — owned NUL-terminated bytes; `new(&str) -> Result<CString,
  NulError>` rejects interior NUL; `as_c_str`, `as_ptr` (obtaining the pointer
  is safe; dereferencing/reading the C string is unsafe), `into_bytes` (state
  whether the trailing NUL is included).
- `std::CStr` — borrowed validated NUL-terminated bytes; `from_bytes` returns
  `Result` when no NUL exists, `to_str` returns `Result` for non-UTF-8,
  `to_bytes` excludes the terminator.
- `std::OsString`/`OsStr` — already exist; make them public and safe.
- `std::path::Path` — borrowed path view (`&OsStr`-like), and `PathBuf` — owned
  path storage. `PathBuf::as_path(&self) -> &Path`,
  `Path::from_os_str(&OsStr) -> &Path`, `PathBuf::from_os_string(OsString) ->
  PathBuf`; both expose `join`, `file_name`, `parent`, `extension`,
  `is_absolute`, `as_os_str`.
- `std::io::Read`/`Write`/`Seek` — safe traits over `&[u8]`/`&mut [u8]` and
  `Result`/`Option`, implemented by file, socket, TLS, HTTP body, buffers,
  compression/archive streams.

These are the foundation for removing `*char` and raw pointer out-params from
library APIs.

#### G. Safe FFI callback trampoline

Provide a small, audited helper for C callbacks:

- stores a `Callback` in a heap context;
- exposes a non-capturing `@extern` trampoline per callback signature;
- frees the context on unregister/drop;
- preserves `Send`/thread rules for callbacks that cross threads.

This removes the need for user code to pass `*mut void` user data to window,
webview, HTTP, and event APIs. The raw trampoline remains inside `std::ffi::raw`
and is covered by ABI tests.

### 3.3 Removed-feature diagnostics

Removed syntax must produce a stable, actionable error, not a confusing parse
error. Use stable codes (not a generic message) with a migration hint:
`REMOVED-001` associated types, `REMOVED-002` external/generic inherent impls,
`REMOVED-003` nested tuple/array patterns, `REMOVED-004` slice
patterns/slice-specific divergent `else`, `REMOVED-005` `Box<[T]>`,
`REMOVED-006` range slicing. Wire `lang/tests/removed/` into
`lang/tests/build.lab`, `scripts/test.sh`, and `lang/docs/safety/audit-modules.txt`
in the same job; a removed-feature code without a wired negative test is not
done.

| Removed syntax | Diagnostic hint |
|---|---|
| `interface I { type Item }` | "associated types are not part of Chemical; add a generic parameter to the interface/function" |
| `Self::Item` / `T::Item` | same |
| `T: Trait<Item = U>` | "associated-type equality constraints are not supported; use a generic parameter" |
| `impl Type { ... }` (external) | "declare methods in the type body or implement an interface with `impl Trait for Type`" |
| `var ((a, b), c) = ...` | "nested tuple patterns are not supported; bind the outer value and destructure it in separate steps" |
| `if(var (a, b) = ...)` | "tuple patterns in conditions are not supported; use a normal condition and flat bindings" |
| `var [a, [b, c]] = ...` | "nested array patterns are not supported; bind a row and destructure it separately" |
| `if(var [a, b] = view)` | "slice patterns are not supported; check `view.size()` and use `view.get(i)`/`view[i]`" |
| `Box<[T]>` | "boxed slices are not part of Chemical; use `vector<T>` for ownership or `[T]`/`&[T]` for a view" |
| `view[a..b]` | "range slicing syntax is not supported; use `view.subview(a, b)`" |

---

## 4. Target safe APIs

### 4.1 API policy

Apply to every shipped library.

**Tier A — user-facing runtime libraries.** `std`, `core`, `fs`, `path`,
`process`, `environment`, `net`, `http`, `tls` (high-level), `json`, `regex`,
`archive`, `image`, `audio`, `font`, `window`, `webview`, `page`, `components`,
`router`, `server`, `test`, `test_env`, `test_report`, `async` (public safe
surface), `osrand`, `datetime`, `uuid`, `encoding`, `compression`, `cbor`,
`mime`, `crypto` (public hash/encoding APIs). `atomic` and the raw async
executor protocol are Tier C.

- No public `unsafe func` except Tier C primitives and the explicit allowlist
  (for example `environment::set_var`, `span::data`, `Window::native_handle`).
  The allowlist is data-reviewed, not a loophole for ordinary API surface.
- No public function parameter/return containing `*T`/`*mut T` for ordinary
  work. Raw access is an explicitly `unsafe func` accessor or lives under a
  `raw`/`ffi`/`sys` module, and is listed in the allowlist.
- No `*char` paths or out-buffer parameters. Use `&Path`, `&str`, `&[u8]`,
  `&mut [u8]`, and returned `Result`/`Option`.
- Owned handles have private fields, `@make`-only construction (no
  `@direct_init` unless every field is safely constructible), and `@delete`.
- `close`/`destroy` is idempotent and nulls the handle.
- Fallible operations return `Result`/`Option`; panicking conveniences are
  clearly named or are `Index`/`unwrap` style.
- `Send`/`Sync` are explicit or correctly derived; guards are `!Send` when
  required; callbacks are bounded.
- Every `unsafe` block has a `// SAFETY:` comment stating the invariant.
- Every public API has a doc comment stating ownership, lifetime, failure, and
  thread behavior.
- Public safe APIs use portable primitives (`&[u8]`, `&mut [u8]`, `&str`,
  `Path`, `Result`, `Option`), not C pointers or platform handles. Raw platform
  handles live in Tier C. This keeps the same API shape viable for future
  JVM/JS/WASM backends and for the native-GC direction; resource types still
  require deterministic `@delete` plus explicit `close` where resources are
  involved.

**Tier B — compiler/plugin/build APIs.** `compiler`, `compiler_runtime`, `lab`,
CBI plugin libraries.

- Provide a safe `compiler::api` facade with `&mut`/`&` parameters, `Option`/
  `Result`, slices, typed handles, and no uninitialized `&mut`/aliasing `&mut`.
- Raw CBI remains in `compiler::ffi`/`lab::raw`; plugin authors do not need it
  for normal macros. The Tier B audit applies the same zero-ordinary-raw-pointer
  rule outside those modules.
- The C ABI boundary is audited by the existing `SAFE-FFI-*` rules.

**Tier C — inherently raw.** `cstd`, `std::raw`, `core::raw`, `tls::raw`,
`tls` cryptographic primitives, `atomic` low-level intrinsics, parser/CBI
internals, `window`/`webview` platform bindings.

- Clearly named `raw`/`ffi`/`sys`.
- Public functions may be `unsafe`/raw, but they are not the normal user API.
- Every raw module has a `SAFETY.md` describing its contract and the safe
  wrapper that contains it.
- Tier C is excluded from the "zero public raw pointers" gate by an explicit
  allowlist in the audit script.

### 4.2 `std` / `core` primitives

#### Containers and views

Target behavior (Rust-familiar, Chemical syntax):

```chemical
// vector<T>
public func get(&self, i : size_t) : Option<&T>
public func get_mut(&mut self, i : size_t) : Option<&mut T>
public func get_copy(&self, i : size_t) : Option<T> where T: Copy
public func <'a> as_span(&'a self) : span<'a, T>
public func <'a> as_mut_span(&'a mut self) : mut_span<'a, T>
public func push(&mut self, v : T)              // panics on OOM (convenience)
public func try_push(&mut self, v : T) : Result<Unit, AllocError>
public func try_reserve(&mut self, n : size_t) : Result<Unit, AllocError>
public func pop(&mut self) : Option<T>
public func remove(&mut self, i : size_t) : Option<T>
public func clear(&mut self)
public func size(&self) : size_t
public func empty(&self) : bool
```

`Index`/`IndexMut` remain panicking. `get`/`get_mut` return `Option`. `get_ptr`
and `data` become `unsafe`/`raw` or are replaced by spans.

```chemical
// span<'a, T> / mut_span<'a, T>; [T]/&[T]/&mut [T] are aliases for these.
public func get(&self, i : size_t) : Option<&T>
public func get_mut(&mut self, i : size_t) : Option<&mut T>
public func get_copy(&self, i : size_t) : Option<T> where T: Copy
public func as_span(&'a self) : span<'a, T>
public func as_mut_span(&'a mut self) : mut_span<'a, T>
public func subview(&self, start : size_t, end : size_t) : Option<span<'a, T>>
public func split_at(&self, at : size_t) : Option<slice_pair<'a, T>>
public func iter(&self) : SpanIter<'a, T>   // safe for-in
public unsafe func data(&self) : *const T   // raw access; allowlisted
```

**View lifetime normalization is a Phase 2 prerequisite.** Today `span<T>`/
`str` store raw pointers without a lifetime; `as_span(&self) -> span<T>` can
outlive the owner. Add the lifetime parameter to `span`/`mut_span`/`str` (and
their `slice_pair`/split/window cursors), make raw construction unsafe, and
map `[T]`/`&[T]`/`&mut [T]`/`&str` to the lifetime-carrying types. This is
safety-critical language/library work, not new sugar. Tests must cover
owner-drop rejection, subview/iterator escape rejection, and safe `&'static`
views. If the lifetime parameter cannot be added without destabilizing MIR,
the fallback is to return `&'a [T]` from safe APIs and keep the raw `span` type
private; it is not acceptable to return a lifetime-less owning view.

```chemical
// string / str / string_view
public func <'a> as_str(&'a self) : &'a str
public func <'a> as_bytes(&'a self) : &'a [u8]
public func get_char(&self, i : size_t) : Option<char>
public func try_append(&mut self, text : &str) : Result<Unit, AllocError>
public func try_reserve(&mut self, n : size_t) : Result<Unit, AllocError>
```

`CString`/`CStr`/`OsString`/`OsStr`/`Path`/`PathBuf` as in §3.2F.

#### `Option`/`Result`

- `Option::take(&mut self) : Option<T>` — no panic on `None`.
- `Option::replace(&mut self, value : T) : Option<T>`.
- `Result::is_ok`/`is_err`/`ok`/`err`/`map`/`map_err`/`unwrap_or`.
- No `unwrap` that panics in a `[no_panic]` module; `unwrap` may exist as a
  clearly named convenience.

#### Memory and allocation

- `Box::new`/`try_new`, `into_inner`, `as_ref`, `as_mut`, `leak`.
- `Rc::new`/`try_new`, `clone`, `downgrade`, `get_mut`, `try_unwrap`.
- `Arc` same with atomic counts.
- `MaybeUninit::uninit`, `write`, `assume_init` (unsafe), `assume_init_drop`
  (unsafe), `as_ptr`.
- `ManuallyDrop::new`, `into_inner`, `take` (unsafe), `destroy` (unsafe).
- `NonNull::new`/`try_new`, `as_ptr`, `as_ref` (unsafe).
- `GlobalAllocator` stays raw/internal; safe containers wrap it.

#### Concurrency

- `Mutex<T>`: `lock() -> Result<MutexGuard<T>, LockError>` or keep panicking
  `lock` plus `try_lock() -> Option<MutexGuard<T>>`; prefer `try_lock` for
  no-panic.
- `RwLock<T>`: `read`/`write`/`try_read`/`try_write`.
- `Condvar`: safe wait with guard; no raw pointer.
- `thread::spawn`/`scope` as in §3.2D.
- `Channel<T>`: `send(T) -> Result<(), SendError<T>>`, `recv() -> Option<T>`,
  `try_recv() -> Result<T, TryRecvError>`; no raw.
- `ThreadPool` safe `submit` with `FnOnce + Send + 'static`; raw `submit` becomes
  `submit_unchecked` in `raw`.
- `Atomics`: typed `AtomicU8/U16/U32/U64` methods; raw intrinsics in `atomic::raw`.

#### Async

- `FutureHandle<'a, T>` public methods are safe/opaque: `poll` via the async
  protocol, `drop` via RAII; raw frame/vtable access only in `core::async::raw`.
- `Waker::from_raw` stays unsafe; `Waker::empty` safe.
- `Pin` safe facade.
- `spawn_send` safe; `spawn`/`spawn_local`/`spawn_blocking` either safe with
  correct bounds or explicitly unsafe with a documented reason.
- `block_on` safe with a `FutureHandle`.

### 4.3 I/O, paths, files, processes, environment

#### `core::io`

```chemical
public interface Read {
    func read(&mut self, buf : &mut [u8]) -> Result<usize, IoError>
    func read_exact(&mut self, buf : &mut [u8]) -> Result<Unit, IoError>
    func read_to_end(&mut self, out : &mut vector<u8>) -> Result<usize, IoError>
    func try_read_to_end(&mut self, out : &mut vector<u8>) -> Result<usize, IoError>
}

public interface Write {
    func write(&mut self, buf : &[u8]) -> Result<usize, IoError>
    func write_all(&mut self, buf : &[u8]) -> Result<Unit, IoError>
    func flush(&mut self) -> Result<Unit, IoError>
}

public interface Seek { ... }
```

Implement for `fs::File`, `net::TcpStream`, `tls::TlsStream`, `http::Body`,
`net::Buffer`, `compression` streams, and `archive` streams. This is the shared
I/O abstraction the library analysis already identified as the biggest gap.

**One error type at the trait boundary.** The traits cannot use `FsError`,
`NetError`, `TlsError`, and `HttpError` interchangeably. Define
`std::io::IoError { kind : IoErrorKind, message : Option<string> }` with
`From<FsError>`/`From<NetError>`/`From<TlsError>`/`From<HttpError>` wrappers, or
make the traits error-parametric (non-object-safe) and choose one dispatch
story. Prefer the single small `IoError` at module boundaries and keep rich
library errors in the library-specific APIs. The success path must not allocate;
store the message lazily (or use a small kind/code and an optional owned detail).

The read/write methods shown on `File`/`TcpStream`/`TlsStream`/`Body` **are the
trait implementations** and return `IoError`; do not add same-name inherent
methods with `FsError`/`NetError`/`TlsError`/`HttpError` that shadow the trait.
Provide `From<IoError> for FsError/NetError/TlsError/HttpError` as well as
`From<...> for IoError` where a library parser wraps I/O. `IoErrorKind` includes
`OutOfMemory`, so `read_to_end`/`read_to_string`/`read_dir`/`args` can report
allocation failure without panicking. `read_to_end`/`read_to_string` remain
convenience methods that may panic on OOM; `try_read_to_end`/
`try_read_to_string` are the `[no_panic]` forms, and `fs::read`/`read_dir`/
`environment::args` must use the fallible container growth internally.

#### `path`

```chemical
public struct Path { ... }        // borrowed path view
public struct PathBuf { ... }     // owned path storage

public func Path::new(value : &str) : &Path
public func Path::from_os_str(value : &OsStr) : &Path
public func PathBuf::as_path(&self) : &Path
public func PathBuf::from_os_string(value : OsString) : PathBuf
public func Path::as_os_str(&self) : &OsStr
public func Path::as_str(&self) : Option<&str>
public func Path::is_absolute(&self) : bool
public func Path::join(&self, other : &Path) : PathBuf
public func Path::file_name(&self) : Option<&OsStr>
public func Path::parent(&self) : Option<&Path>
public func Path::extension(&self) : Option<&OsStr>
public func Path::normalize(&self) : Result<PathBuf, PathError>
```

Free functions `basename`/`dirname`/`join`/`normalize` remain only if they are
thin safe wrappers; no `*char`/out-buffer forms.

`PathBuf` stores POSIX bytes (NUL-terminated) or Windows UTF-16, rejects
interior NUL, and caches/owns the C-string form so a syscall does not allocate
on every call. `as_str()` is fallible on non-UTF-8 POSIX paths; use
`as_os_str()` for lossless access. `file_name`/`parent`/`extension` return
borrowed views with explicit lifetimes tied to the path.

#### `fs`

```chemical
public struct File { private ... }
public struct OpenOptions { ... }
public struct Metadata { ... }
public struct DirEntry { ... }

public func File::open(path : &Path, opts : &OpenOptions) -> Result<File, FsError>
public func File::create(path : &Path) -> Result<File, FsError>
public func File::read(&mut self, buf : &mut [u8]) -> Result<usize, IoError>
public func File::read_exact(&mut self, buf : &mut [u8]) -> Result<Unit, IoError>
public func File::write(&mut self, buf : &[u8]) -> Result<usize, IoError>
public func File::write_all(&mut self, buf : &[u8]) -> Result<Unit, IoError>
public func File::seek(&mut self, pos : SeekFrom) -> Result<u64, IoError>
public func File::flush(&mut self) -> Result<Unit, IoError>
public func File::metadata(&self) -> Result<Metadata, FsError>

public func read(path : &Path) -> Result<vector<u8>, FsError>
public func write(path : &Path, data : &[u8]) -> Result<Unit, FsError>
public func read_to_string(path : &Path) -> Result<string, FsError>
public func create_dir_all(path : &Path) -> Result<Unit, FsError>
public func remove_file(path : &Path) -> Result<Unit, FsError>
public func remove_dir_all(path : &Path) -> Result<Unit, FsError>
public func read_dir(path : &Path) -> Result<vector<DirEntry>, FsError>
```

`FsError` stores `string`, not `*char`. `File` is `@make`-only, private handle,
idempotent `close`, `@delete`.

#### `process`

```chemical
public struct Command { private ... }
public struct Child { private ... }
public struct Output { ... }
public struct ExitStatus { ... }

public func Command::new(program : &str) : Command
public func Command::arg(&mut self, arg : &str) -> &mut Command
public func Command::args(&mut self, args : &[&str]) -> &mut Command
public func Command::env(&mut self, key : &str, value : &str) -> &mut Command
public func Command::current_dir(&mut self, dir : &Path) -> &mut Command
public func Command::stdin(&mut self, stdio : Stdio) -> &mut Command
public func Command::stdout(&mut self, stdio : Stdio) -> &mut Command
public func Command::stderr(&mut self, stdio : Stdio) -> &mut Command
public func Command::output(&mut self) -> Result<Output, ProcessError>
public func Command::status(&mut self) -> Result<ExitStatus, ProcessError>
public func Command::spawn(&mut self) -> Result<Child, ProcessError>

public func Child::wait(&mut self) -> Result<ExitStatus, ProcessError>
public func Child::try_wait(&mut self) -> Result<Option<ExitStatus>, ProcessError>
public func Child::kill(&mut self) -> Result<Unit, ProcessError>
public func Child::write_stdin(&mut self, data : &[u8]) -> Result<Unit, ProcessError>
public func Child::close_stdin(&mut self) -> Result<Unit, ProcessError>
public func Child::id(&self) -> u32
```

`Output::stdout_bytes() -> [u8]`, `stdout_str() -> Option<&str>` (UTF-8
checked), same for stderr. No `ProcessConfig` with public raw fields.

`Child` semantics must be defined: `@delete` does not silently kill a running
process unless documented; it closes owned pipes and avoids zombie leaks;
`wait` is idempotent (returns the cached status after the first wait);
`try_wait` returns `Option`; `write_stdin`/`close_stdin` own a safe stdin
handle; stdout/stderr are owned `Read` streams when piped. If `Child` is
dropped while running, the chosen behavior (wait, kill, or detach) must be
documented and tested.

#### `environment`

```chemical
public func args() : vector<string>
public func var(name : &str) -> Result<string, EnvError>
public func var_os(name : &str) -> Option<OsString>
public unsafe func set_var(name : &str, value : &str) -> Result<Unit, EnvError>
public unsafe func remove_var(name : &str) -> Result<Unit, EnvError>
public func current_dir() -> Result<PathBuf, EnvError>
public unsafe func set_current_dir(dir : &Path) -> Result<Unit, EnvError>
public func temp_dir() -> PathBuf
```

Environment mutation is process-global and races with other threads, so
`set_var`/`remove_var`/`set_current_dir` are explicit `unsafe` boundaries (or
require a single-threaded proof). Prefer `Command::env`/`Command::current_dir`
for child processes. Reads (`var`, `current_dir`) stay safe.

### 4.4 Network, HTTP, TLS

#### `net`

```chemical
public struct TcpListener { private ... }
public struct TcpStream { private ... }
public struct SocketAddr { ... }

public func TcpListener::bind(addr : &str, port : u16) -> Result<TcpListener, NetError>
public func TcpListener::accept(&mut self) -> Result<TcpStream, NetError>
public func TcpStream::connect(addr : &str, port : u16) -> Result<TcpStream, NetError>
public func TcpStream::read(&mut self, buf : &mut [u8]) -> Result<usize, IoError>
public func TcpStream::read_exact(&mut self, buf : &mut [u8]) -> Result<Unit, IoError>
public func TcpStream::write(&mut self, buf : &[u8]) -> Result<usize, IoError>
public func TcpStream::write_all(&mut self, buf : &[u8]) -> Result<Unit, IoError>
public func TcpStream::set_read_timeout(&mut self, ms : u64) -> Result<Unit, IoError>
public func TcpStream::shutdown(&mut self, how : Shutdown) -> Result<Unit, IoError>
@delete func TcpStream::delete(&mut self)
```

`Socket` as a bare `usize` is removed from the public API. Async read/write take
`&mut [u8]` and return `FutureHandle` with correct borrow lifetimes; add
`AsyncRead`/`AsyncWrite` (or an equivalent lifetime-carrying async I/O trait)
instead of `*mut u8`/`'static` futures. If an async path cannot be made safe in
this release, leave it in `net::raw` and document the limitation; do not expose
a safe-looking async API with raw buffers.

#### `http`

```chemical
public struct Client { ... }
public struct RequestBuilder { ... }
public struct Response { ... }
public struct HeaderMap { ... }
public struct Body { private ... }

public func Client::get(&self, url : &str) -> Result<Response, HttpError>
public func Client::request(&self, req : &Request) -> Result<Response, HttpError>
public func HeaderMap::get(&self, name : &str) -> Option<&str>
public func Body::read(&mut self, buf : &mut [u8]) -> Result<usize, IoError>
public func Body::read_to_end(&mut self, out : &mut vector<u8>) -> Result<usize, IoError>
public func Body::json<T>(&mut self) -> Result<T, HttpError> where T: Deserializer<T>
public func <R : Read> parse_request(bytes : &[u8], conn : &mut R) -> Result<Option<Request>, HttpError>
```

`Body` owns its state; no public raw buffer/TLS fields. Server connections use
a safe `Transport` enum rather than a raw pointer or a `dyn Read` object:

```chemical
public variant Transport {
    Plain(TcpStream)
    Tls(TlsStream)
    // Async transport, if kept, is a separate variant with its own lifetime.
}
```

`Transport` implements `Read`/`Write` by switching; `Request`/`Response` stay
non-generic to avoid monomorphization at every layer. `dyn Read` is not the
default because it is conservatively `!Send` and `[no_panic]` rejects dynamic
dispatch. If an async transport is kept, give it an explicit
`AsyncRead`/`AsyncWrite` surface with real `&mut [u8]` borrows; otherwise park
it in `http::raw` and document that limitation.

#### `tls`

```chemical
public struct TlsConfig { private ... }
public struct Certificate { private ... }
public struct CertificateChain { private ... }
public struct PrivateKey { private ... }
public struct TlsStream { private ... }

public func TlsConfig::client() -> TlsConfig
public func TlsConfig::server() -> TlsConfig
public func TlsConfig::set_ca_chain(&mut self, chain : &CertificateChain) -> Result<Unit, TlsError>
public func TlsConfig::set_alpn(&mut self, protocols : &[&str]) -> Result<Unit, TlsError>
public func TlsConfig::set_own_chain(&mut self, chain : &CertificateChain) -> Result<Unit, TlsError>
public func TlsConfig::set_own_key(&mut self, key : &PrivateKey) -> Result<Unit, TlsError>

public func Certificate::from_der(der : &[u8]) -> Result<Certificate, TlsError>
public func Certificate::from_pem(pem : &[u8]) -> Result<Certificate, TlsError>
public func CertificateChain::from_der(der : &[u8]) -> Result<CertificateChain, TlsError>
public func CertificateChain::from_pem(pem : &[u8]) -> Result<CertificateChain, TlsError>
public func CertificateChain::push(&mut self, cert : Certificate)
public func Certificate::subject_cn(&self) -> Option<&str>
public func Certificate::verify_hostname(&self, host : &str) -> bool // SAN-first, CN fallback only if policy allows
public func load_system_ca_bundle() -> Result<CertificateChain, TlsError>

public func TlsStream::connect(host : &str, port : u16, cfg : &TlsConfig) -> Result<TlsStream, TlsError>
public func TlsStream::accept(stream : TcpStream, cfg : &TlsConfig) -> Result<TlsStream, TlsError>
public func TlsStream::wrap(stream : TcpStream, cfg : &TlsConfig) -> Result<TlsStream, TlsError>
public func TlsStream::read(&mut self, buf : &mut [u8]) -> Result<usize, IoError>
public func TlsStream::write(&mut self, data : &[u8]) -> Result<usize, IoError>
public func TlsStream::close_notify(&mut self) -> Result<Unit, IoError>
public func TlsStream::alpn_protocol(&self) -> Option<&str>
public func TlsStream::peer_certificate(&self) -> Option<&Certificate>
@delete func TlsStream::delete(&mut self)
```

Server-side accept, `TlsConfig::server()`, `set_own_chain`, wrapping an
existing `TcpStream`, ALPN result, and peer-certificate access are part of the
safe API, not optional. The async TLS transport must expose `AsyncRead`/
`AsyncWrite` (or be explicitly parked in `tls::raw` with a documented reason).
Do not claim a safe TLS client/server API if the server path is missing.

Move mbedTLS-shaped contexts/functions to `tls::raw` (Tier C). Public crypto
primitives get safe wrappers:

```chemical
public func sha256(data : &[u8]) -> [32]u8
public struct Sha256 { ... } // update(&[u8]), finalize() -> [32]u8
public func hmac_sha256(key : &[u8], data : &[u8]) -> [32]u8
public func constant_time_equal(a : &[u8], b : &[u8]) -> bool
public func x25519(secret : &[32]u8, peer_public : &[32]u8) -> Result<[32]u8, CryptoError>
```

### 4.5 Data, media, crypto

- `encoding`: functions take `&[u8]`/`&str` and return `Result<string>` /
  `Result<vector<u8>>`; no out buffers.
- `crypto`: one-shot functions return fixed arrays; streaming contexts own
  state; no digest out-params.
- `compression`: `compress(&[u8]) -> Result<vector<u8>>`,
  `decompress(&[u8]) -> Result<vector<u8>>`; streaming readers implement
  `Read`.
- `archive`: `ZipArchive::open(&Path)`, `from_bytes(&[u8])`,
  `entries(&self) -> &[ArchiveEntry]`, `entry_count(&self)`,
  `read_entry(&mut self, index : size_t) -> Result<vector<u8>, ArchiveError>`,
  `read_file(&mut self, name : &str)`, `extract(&mut self, &Path)`. Do not take
  `&ArchiveEntry` from `entries()` while taking `&mut self`; use an index. Every
  extract must reject `..`, absolute paths, and symlink escapes (zip-slip).
  No `*mut` handles/out-params.
- `image`: `Image::from_png(&[u8])`, `from_bmp`, `from_ppm`, `load(&Path)`,
  `encode_png(&self) -> Result<vector<u8>>`, `save(&self, &Path)`, and methods
  `width/height/channels/pixel/get/set/crop/flip/blit`, plus
  `pixels(&'a self) -> &'a [u8]`/`pixels_mut(&'a mut self) -> &'a mut [u8]` for
  bulk codec work. No `*mut Image`.
- `audio`: `Audio::from_wav(&[u8])`, `load`, `encode_wav`, `save`, methods
  `duration/trim/volume/mix/append/resample`, plus `samples(&'a self) -> &'a
  [f32]`/`samples_mut(&'a mut self) -> &'a mut [f32]` (or the concrete sample
  type) for bulk processing. No `*mut Audio`.
- `font`: `Font::load(&[u8])`, `from_path`, metrics methods; text uses `&str`
  (UTF-8) and returns metrics. No `*mut Font`/`*char`.
- `regex`: `public func <'a> find(&self, text : &'a str) : Option<Match<'a>>`,
  `public func <'a> captures(&self, text : &'a str) : Option<Captures<'a>>`
  where the match/capture views borrow the searched text (no allocation);
  `replace(&self, text : &str, replacement : &str) -> string`;
  `split(&self, text : &str) -> vector<string>` (owned) or a lifetime-bound
  view type with an explicit allocation trade-off. No out-params.
- `json`: public API already mostly safe; remove raw string parsers from the
  public surface; `json_escape_into(&mut string, &str)`; `JsonValue` remains.
- `mime`/`datetime`/`uuid`/`osrand`: use `&str`/`&[u8]` and `Result`.
- `cbor`: `map_value_by_text`/`array_value_at` return `Option<&CborValue>`, not
  `*mut`.

### 4.6 Web/UI

- `window`:
  - `Window::new(title : &str) -> Result<Window, WindowError>`;
  - all `window_*` free functions become methods on `&mut Window` / `&Window`;
  - `set_title(&str)`, `size()`, `set_size`, `position`, `set_position`,
    `show/hide/focus/close`, `set_cursor`, `set_icon(&Path)`, `set_opacity`,
    `set_always_on_top`, `set_decorated`, `dpi`, `scale_factor`;
  - callbacks: `on_resize(F)`, `on_close(F)`, `on_focus(F)`, `on_event(F)`,
    `on_drop(F)`, `on_command(F)`, `set_timer`; `F` is `Callback<...>` (or the
    existing witness form if `Callback` is not proven) with an owned capture;
    no `*mut void` user data;
  - every registration has an `off_*`/unregister path; the trampoline owns
    exactly-one free and the `Window`/`WebView` destructor unregisters and frees
    all contexts; in-flight callbacks are cancelled/quiesced before teardown;
  - `Window` and `WebView` are `!Send` (UI-thread-affine) and must not be
    constructed or dropped from a different thread;
  - `unsafe func native_handle(&self) -> *mut void` stays an explicit raw
    accessor (or moves to `window::raw`); it is allowlisted, not part of normal
    window use.
- `webview`:
  - `WebView::new(...) -> Result<WebView, WebViewError>`;
  - `load_url(&str)`, `load_html(&str)`, `evaluate(&str)`, `set_title(&str)`,
    `set_size`, `show/hide`, `run`, `stop`, `bind(name : &str, F)` (returns a
    registration handle), `unbind(&str)`, `return_value(&str, status, &str)`;
  - callbacks use the safe stored-callback form; raw platform interfaces under
    `webview::raw`; the same unregister/free/`!Send` rules apply.
- `page`:
  - `HtmlPage::append_html(&str)`, `append_js(&str)`, `escape_html(&str)`;
  - parameter objects: `add_parameter<T : PageParam>(&mut self, key : &str,
    value : &T)`; no `*mut void`;
  - `add_parameter` must use one escaping contract (JSON for data, JS-string
    escaping for embedded strings), reject/escape `'`/`\`/`</script>`, and have
    XSS tests. If raw emission is needed, name it `append_js_raw` and make the
    escape requirement explicit;
  - `SharedAssets` is an owned `Rc`/handle with private fields.
- `components`/`router`/`server`: public methods use `&str`/`&[u8]`, typed
  handles, and `Result`; no raw pointers in user-facing APIs.

### 4.7 Compiler/plugin/build APIs

- `compiler::api` safe facade:
  - `Parser::current(&self) -> Option<&Token>`, `increment()`;
  - `ASTBuilder::allocate<T>() -> &mut MaybeUninit<T>` (arena storage, explicit
    initialization) or `allocate_default<T>() -> &mut T where T: Default`, and
    `allocate_str(&str) -> &str`/`InternedStr`. Do **not** return `&mut T` over
    uninitialized arena memory, and do not return `MaybeUninit<T>` by value
    (that is a detached uninitialized local, not arena storage);
  - `SourceProvider::peek() -> Option<char>`, `read_char()`;
  - `SymResLinkBody::resolve(&str) -> Option<&ASTNode>` or `NodeId`; do not
    hand out two live `&mut` to the same arena node;
  - `PtrVec<T>` as a safe slice/index type with no aliasing `&mut`; return
    shared slices or typed handles;
  - `ASTDiagnoser::error(&str, loc)`.
- `lab` safe facade:
  - `ModuleId`/`JobId` typed handles instead of `*mut Module`/`*mut LabJob`;
  - `BuildContext::new_module(...) -> ModuleId`;
  - `lab::run(&str) -> Result<string, LabError>` (do not hide failure).
- `compiler_runtime` raw C ABI stays internal; safe wrappers used by build.lab.
- CBI plugins continue to compile against the safe facade; raw CBI only for
  compiler internals.

### 4.8 `cstd` and FFI

- `cstd` remains the raw C binding surface (Tier C). Every function is
  `@extern`/unsafe-call; it is not a user-facing safe API.
- Add safe wrappers for OS operations used by `fs`, `net`, `process`, `window`,
  `webview`, `tls`, and `time`.
- Validate at the boundary: null pointers, lengths, alignment, UTF-8/UTF-16,
  integer ranges, handle validity, and ownership transfer.
- Every wrapper documents whether it borrows, copies, or takes ownership.
- `// SAFETY:` comments state the C contract.

### 4.9 API migration table (representative)

| Current API | Target API |
|---|---|
| `path::basename(p : *char, out : *mut char, out_len)` | `Path::file_name(&self) -> Option<&OsStr>` / `basename(&Path) -> Result<OsString>` |
| `fs::read_entire_file(path : *char)` | `fs::read(path : &Path) -> Result<vector<u8>, FsError>` |
| `fs::file_read(f : *mut File, buf : *mut u8, len)` | `File::read(&mut self, buf : &mut [u8]) -> Result<usize, IoError>` |
| `process::wait(child : *mut ChildProcess)` | `Child::wait(&mut self) -> Result<ExitStatus, ProcessError>` |
| `net::recv_all(s : Socket, buf : *mut u8, cap)` | `TcpStream::read_exact(&mut self, buf : &mut [u8])` |
| `http::Body.read(dst : *mut u8, cap)` | `Body::read(&mut self, buf : &mut [u8])` |
| `http::parse_request_from_bytes(buf : *u8, n)` | `parse_request(bytes : &[u8], ...)` |
| `tls::ssl_handshake(ssl : *mut SSLContext)` | `TlsStream::connect(...)`; raw in `tls::raw` |
| `tls::x25519_compute_shared(priv, peer, out)` | `x25519(&secret, &peer) -> Result<[32]u8>` |
| `archive::open_zip(path : *char, out : *mut ZipArchive)` | `ZipArchive::open(&Path) -> Result<Self>` |
| `image::load_png(path : *char)` | `Image::load(&Path)` / `Image::from_png(&[u8])` |
| `audio::load_wav(path : *char)` | `Audio::load(&Path)` / `Audio::from_wav(&[u8])` |
| `font::font_measure_text(font : *mut Font, text : *char, len)` | `Font::measure_text(&self, text : &str) -> TextMetrics` |
| `regex::find(text, start : *mut i64, end : *mut i64)` | `Regex::find(&self, text : &str) -> Option<Match>` |
| `std::vector::get_ptr(i) -> *mut T` | `get(i) -> Option<&T>` / `get_mut(i) -> Option<&mut T>` |
| `std::span::get(i) -> *T` | `get(i) -> Option<&T>` / `get_copy(i) -> Option<T>` |
| `std::string` raw constructor | `string::from(&str)` / `try_from_utf8`; raw under `std::raw` |
| `Option::take()` panic on `None` | `Option::take(&mut self) -> Option<T>` |
| `window_create(*mut Window)` | `Window::new(...) -> Result<Window, WindowError>` |
| `webview_create(*mut WebView)` | `WebView::new(...) -> Result<WebView, WebViewError>` |
| `page.add_parameter_object(*mut void)` | `page.add_parameter<T : PageParam>(&str, &T)` |
| `compiler::Parser.getToken() -> *mut Token` | `Parser::current() -> Option<&Token>` |
| `lab::new_module(...) -> *mut Module` | `BuildContext::new_module(...) -> ModuleId` |

### 4.10 API tests and contracts

For every Tier A library:

1. **Positive API test** using only the safe surface, compiled on TCC and LLVM.
2. **Forgery test** attempting to construct an invariant-bearing handle from
   an integer/raw pointer; must fail `SAFE-VIS-001`/`SAFE-UNSAFE-001`.
3. **Misuse test** for null/closed/out-of-range/UTF-invalid input; must return
   `Err`/`None`, never abort or corrupt.
4. **Drop test** for exactly-once cleanup; ASAN/leak check.
5. **Thread test** for `Send`/`Sync` bounds and guard `!Send`; TSAN where
   applicable.
6. **No-panic test** for the fallible surface in a `[no_panic]` module.
7. **Public API snapshot** in `lang/docs/safety-sharpen/api/<lib>.txt`; a change
   requires review.
8. **Unsafe boundary map** in `lang/docs/safety-sharpen/unsafe/<lib>.md`; every
   unsafe block cites its invariant and wrapper.

### 4.11 Performance rules for safe APIs

- Borrowed views (`&[u8]`, `&mut [u8]`, `&str`) instead of copies.
- `Result`/`Option` must not allocate on the success path. Library error types
  should be a small kind/code plus an optional lazily-boxed message; a
  a `Result<usize, E>` with an inline owned `string` is a large sret on every
  `read`/`write` and is not acceptable for the hot I/O path.
- Error conversion at `io::Read`/`Write` boundaries must preserve the kind and
  avoid allocating unless a message is actually requested.
- Generic callbacks (`F: Fn*`) for hot paths; `std::function`/`Callback` only
  when type erasure is required.
- `@inline` small wrappers only; do not inline large bodies.
- No hidden allocation in constructors; `try_*` uses the existing allocator.
- Benchmark file I/O, sockets, HTTP, TLS handshake, JSON, regex, image
  encode/decode, and vector/string operations against the pre-change baseline.
- Any >5% runtime regression on a core benchmark must be justified or fixed
  before the release gate.
- Measure compile time/RSS and generic instantiation counts for std-heavy code;
  generic `Read`/`Write`/`Body<T>` layers must not multiply monomorphizations.
  Keep erasure/enums at module boundaries and add a compile-time/RSS gate to
  Phase 2, not only Phase 3.

---

## 5. Execution phases

> **Execution contract.** A phase is the unit of work. Do not stop after one
> job, one commit, or one library. Run the jobs in order, commit logical
> checkpoints, update `STATUS.md` after each job, and continue until the phase
> exit criteria are met or a genuine blocker is recorded. If blocked on one job,
> record the exact command/output in `BLOCKED.md` and continue with independent
> jobs. Do not ask the user to type "continue" between jobs.
>
> **Human checkpoints are explicit:** (a) the §2.5 decisions before Phase 1
> starts; (b) the Phase 1 → Phase 2 boundary (removal set and API expressibility
> audit); (c) before tagging the Phase 2 release. Everything else is mechanical
> and continues without a prompt. A genuine blocker is: an ambiguous public API
> decision that needs the user, two failed fix attempts on the same gate with the
> same root cause, a required sign-off/reviewer decision, or an environment
> failure that cannot be worked around; record it with command/output and move
> to independent work.

### Phase 1 — Baseline and sharpen (breaking)

**Goal:** repair the release build, remove the low-value feature families, make
the language surface smaller and clearer, and establish size/compile-time
measurements.

#### Jobs

**P1.1 — Repair the release build and baseline.**
- Fix `GenericInstantiator::saveRequirement/restoreRequirement` for non-`DEBUG`
  builds (guard the optional path or always use optional; choose one and test
  both).
- Rebuild release `TCCCompiler`; record size, `size` sections, and top symbols.
- Record compile-time/RSS baseline for small, std-heavy, and tests builds.
- Add `scripts/safety-sharpen-metrics.sh` and `lang/docs/safety-sharpen/metrics.json`.
- Phase 1 is a measurement/removal phase: release size must not grow above the
  starting baseline, and every removal job records its delta. Do not assume
  syntax removal shrinks `MIRLowerCall`; array drop selectors are driven by
  initialization/drop, not only pattern syntax. The measured selector
  consolidation is P1.7. The mechanical
  **≤5,000,000-byte gate is enforced at the Phase 2 release gate** (4 MB goal).
  If removals alone do not reach it, P2.8 includes the required consolidation
  before release. Over-ceiling blocks the release; there is no silent waiver.
  The only exception path is an explicit user decision to ship a temporary
  first release above 5 MB with a dated reduction plan, recorded in
  `STATUS.md` before tagging.
- Acceptance: release build succeeds; metrics recorded; no behavior change;
  Phase 1 size is not above the starting baseline.

**P1.2 — Keep `dyn`/`DynBox` and harden dynamic dispatch.**
- **Do not remove** `dyn`, dynamic calls, object safety, upcasting, `DynBox`, or
  the dyn auto-trait surface. They are required for runtime/plugin
  polymorphism and were present before the safety wave.
- Add contract tests for `DynBox` ownership/drop, `dyn` view borrowing, dynamic
  call dispatch, and upcasting.
- Document the safe `dyn` surface. `dyn I + Send`/`+ Sync`/`+ Unpin` construction
  checks already exist (`SAFE-SEND-003`); keep them. The remaining limitation is
  unbound `dyn` being conservatively `!Send`/`!Sync`/`!Unpin`, and
  `Box<dyn I>`/`Arc<dyn I>` not crossing threads; that is a hardening item, not
  a reason to remove the feature.
- **Enum-sync rule:** if any `ASTNodeKind` or `TokenType` value is removed in a
  later job, remove it from the matching Chemical binding file in the same
  commit (`lang/libs/compiler/src/ast/base/ASTNodeKind.ch`,
  `lang/libs/compiler/src/ChemicalTokenType.ch`) or keep the slot reserved.
  Never shift enum values silently; CBI plugins are TCC-compiled and an
  off-by-one crashes every plugin.
- Acceptance: `dyn`/`DynBox` tests pass; no dynamic-call regressions; compiler
  size recorded.

**P1.3 — Remove associated types and equality constraints.**
- Delete associated-type declarations/bindings/projections/bounds/equality,
  `Self::Item`/`T::Item`, generic-instantiation machinery, tests/docs.
- Rewrite `lang/tests/src/basic/interface/associated_projection.ch` and the
  associated-type negative cases in `lang/tests/negative/src/main.ch` to a
  generic-parameter interface test; keep normal `type Alias = T` aliases.
- Create `lang/tests/removed/` and wire it into `lang/tests/build.lab`,
  `scripts/test.sh` (as `--removed`), and the audit module list in this first
  removal job, so every later removal commit is green. Add negative tests for
  `type Item`, `Self::Item`, `T::Item`, `T: Trait<Item = U>` with `REMOVED-001`.
- Acceptance: JSON/generic impls/tests unaffected; removed suite runs; both
  backends green.

**P1.4 — Keep tuple structs/unit tuple; remove external/generic inherent impls.**
- **Keep** tuple structs (`struct Point(int, int)`), positional construction,
  `._N` access, tuple-struct patterns, and the unit tuple `()`.
- Keep tuple-struct **in-body methods** and **trait impls**. Remove only
  external inherent `impl Type {}` and generic inherent `impl<T> Type<T> {}`
  resolution; do not delete the tuple-struct method tests.
- Add positive tests for in-body tuple-struct methods and a trait impl on a
  tuple struct; rewrite the external/generic inherent cases in
  `lang/tests/common/src/values/tuple.ch` to trait impls/in-body methods; add
  negative tests/migration hints for external/generic inherent impls.
- Acceptance: tuple struct/unit tuple tests (including in-body methods and trait
  impls) pass; external/generic inherent impls reject clearly; both backends
  green.

**P1.5 — Remove nested/condition patterns; keep flat and variant forms.**
- Remove nested tuple patterns, `if(var (a,b))`, nested fixed-array patterns,
  and slice condition/statement patterns.
- Remove only the slice/array-specific divergent-`else` lowering. **Keep**
  ordinary variant/`Option`/`Result` patterns and their `else` branches,
  `PatternMatchExpr`/`PatternMatchExprNode`/`PatternMatchIdentifier`/
  `PatternElseExprKind`, `ValueKind::PatternMatchExpr`, and `json_cbi`-generated
  `Ok`/`Err` patterns. The `else unsafe { unreachable }` safety form stays.
- Keep flat tuple destructuring and flat fixed-array exact/rest destructuring.
- Rewrite the removed-shape tests explicitly: list each test file and its
  replacement; do not silently delete safety coverage. At minimum cover
  `lang/tests/common/src/values/tuple.ch` nested tuple/array cases,
  `lang/tests/src/stdlib/datatype/slices.ch` range/slice-pattern cases,
  `lang/tests/libs/slice_pattern_test.ch`, and the nested array drop cases in
  `lang/tests/safety/src/main.ch`. Add equivalent nested-index move/drop tests
  (`values[0][0]`, row extraction, etc.) so the MIR move/drop paths remain
  exercised.
- Add negative tests for each removed form, plus positive regressions for
  `if(var Some(v) = x) ... else ...` and CBI-built `Ok`/`Err`.
- Acceptance: playground-style `var [value, set] = useState(...)` still works;
  `var (a,b) = pair` still works; variant patterns with `else` still work;
  removed forms reject with hints; safety/MIRTests counts do not silently fall.

**P1.6 — Remove `Box<[T]>`/`boxed_slice` and range-slicing syntax.**
- Delete `std::boxed_slice`, the parser alias, tests/docs. Rewrite the
  `Box<[T]>` cases in `lang/tests/src/stdlib/datatype/slices.ch` and
  `lang/docs/slices.md` to `vector<T>`/`span<T>`; keep `Box<T>`.
- Delete range-slicing syntax (`view[a..b]`, `[..b]`, `[a..]`, `[..]`); rewrite
  `lang/tests/src/stdlib/datatype/slices.ch` to `subview`/`split_at`. Keep the
  methods.
- Acceptance: `vector<T>`/`[T]` covers ownership/borrowing; range syntax
  rejects with a migration hint; both backends green.

**P1.7 — Release-gating size consolidation.**
- Start the largest measured size win in Phase 1, not post-release: replace the
  per-dimensional array drop selectors in `compiler/mir/MIRLowerCall.cpp` with a
  generic runtime drop-path descriptor/helper, keeping constant-index fast paths.
- Move non-essential tooling/metadata out of the core binary where safe;
  compress strings/tables; strip the release artifact. Do not weaken any safety
  check.
- Run the full safety/move/drop suites (including nested-index drop tests) after
  the consolidation. Record the measured release-size delta and the remaining
  path to 5 MB.
- Acceptance: release size is at or below 5,000,000 bytes, or the remaining
  measured work is **≤100 KB with a concrete task list and expected delta** that
  P2.8 can finish before tagging; otherwise continue P1.7. No safety regression.

**P1.8 — Reconcile docs/skills and freeze new syntax.**
- Split the safety plan into current-status/design/history; update
  `AGENTS.md`, `language_goals`, `safety` skill, feature docs.
- Remove claims that generic impls are unsupported; remove removed features.
- Specifically update: `AGENTS.md:48` (`Box<[T]>` claim), safety skill items
  13/14/15 (`Box<[T]>`, associated types, external inherent methods), and
  `lang/docs/language-goals.md:441-495` (generic impls unsupported). Make the old
  `lang/docs/safety-implementation-plan.md` a short redirect/stub to the split
  current-status/design/history docs.
- Add `lang/docs/safety-sharpen/REMOVED.md` and `BREAKING.md`.
- Verify/expand `lang/tests/removed/` (created in P1.3) with stable
  `REMOVED-*` negative tests; keep it wired into `lang/tests/build.lab`,
  `scripts/test.sh`, and the audit module list.
- Add a feature-policy check to `AGENTS.md`: no new syntax without a safe API.
- Acceptance: one authoritative current-status doc; no stale feature claims;
  removed-feature tests run in CI.

#### Phase 1 exit criteria

- Release `TCCCompiler` builds; its exact size, per-removal delta, and
  distance-to-5 MB are recorded. Phase 1 must not grow the release over its
  starting size; the hard **≤5,000,000-byte gate is at Phase 2 release**.
- Removed features are gone from parser/typecheck/symres/MIR/codegen/tests/docs,
  verified by a mechanical feature-absent grep/CI check (not only tests).
- Associated types, external/generic inherent impls, nested/slice patterns,
  `Box<[T]>`, and range slicing reject with clear diagnostics.
- `dyn`/`DynBox`, tuple structs, unit tuple, core tuples, flat tuple
  destructuring, flat fixed-array destructuring, slices/`str`, generic impls,
  and all safety enforcement remain and have contract tests.
- **Kept machinery is explicit:** ordinary type aliases (`TypealiasStmt`),
  variant/`Option`/`Result` patterns (`PatternMatchExpr`/
  `PatternMatchExprNode`/`PatternMatchIdentifier`/`PatternElseExprKind`/
  `ValueKind::PatternMatchExpr`), and `json_cbi`-generated patterns stay. No
  kept `ASTNodeKind`/`TokenType` value shifts; removed enum slots are reserved
  or updated in the Chemical binding files in the same commit, and the CBI
  enum-sync check runs in CI.
- TCC/LLVM main, safety, negative, async, libs, plugins, interpret, `MIRTests`,
  TSAN, audit gates green (counts updated).
- `--regexp`, `--process`, and `--server` pass; `--webview`/`--universal` run
  where the display/toolkit is available, otherwise compile-only is recorded as
  an explicit environment exception. `--tls` remains opt-in but must be run once
  before the Phase 2 release gate.
- **API expressibility audit:** every signature in §4 is checked against the
  retained feature set; any signature that needs a removed feature is either
  rewritten or recorded as a separately approved reintroduction. No Phase 2 API
  may discover a missing feature after Phase 1.
- Size and compile-time deltas are recorded; no regression justification needed
  for removals.

---

### Phase 2 — Safe APIs and release (breaking)

**Goal:** every shipped user-facing library exposes safe APIs that meet the
policy in §4.1; the safe-API release is tagged at the end.

**Per-job docs rule:** every Phase 2 job updates the affected library README,
`SAFETY.md`, API snapshot, and examples in the same commit. Docs drift is a
failed job, not a cleanup task for P2.8.

#### Jobs

**P2.1a — Error taxonomy and I/O traits.**
- Define `std::io::IoError`/`IoErrorKind` (small kind/code + optional lazily
  owned message) and `From` conversions for `FsError`/`NetError`/`TlsError`/
  `HttpError`.
- Define `Read`/`Write`/`Seek` over `&[u8]`/`&mut [u8]` with `Result` and no
  success-path allocation; decide the dispatch story (enum/generic, not a
  raw-pointer or default `dyn` object).
- Add conversion/size/layout tests and a benchmark proving the success path is
  zero-allocation.
- Acceptance: one error type at the trait boundary; `Result<usize, IoError>` is
  small enough for the hot path; all library error conversions tested.

**P2.1 — `std`/`core` foundations.**
- Implement `Result`/`Option` safe methods, `get`/`try_*` vector/string
  operations, `as_span`/`as_str`, `CString`/`CStr`, `OsString`/`OsStr`,
  `Path`/`PathBuf`, `io::Read`/`Write`/`Seek` with one `IoError`, concrete
  `PinBox`/`PinRef`/`PinMut`, `MaybeUninit`/`ManuallyDrop`/`NonNull` cleanup.
- **View-lifetime normalization:** add the lifetime parameter to
  `span`/`mut_span`/`str` (or return `&'a [T]`/`&'a str` from safe APIs and keep
  the raw view private). This is a Phase 2 prerequisite; do not expose
  lifetime-less owning views.
- Remove/relocate raw access to `std::raw`/`core::raw`; explicit `unsafe`
  accessors are allowlisted.
- Add `try_*` allocation APIs and failing-allocator tests.
- Annotate the fallible core path that the Phase 2 no-panic sample uses with
  `[no_panic]` (or provide an equivalent explicit effect proof); full
  generic/async/interface effect propagation remains P3.1.
- Acceptance: `std` public API audit zero raw pointers outside `std::raw`;
  all containers/views/pointers have contract tests; the Phase 2 no-panic sample
  compiles using only explicitly proven fallible APIs.

**P2.2 — `path`, `fs`, `process`, `environment`.**
- Rewrite to `Path`/`PathBuf`, `&[u8]`, `Read`/`Write`, `Result`, small
  error kinds with optional lazily-owned messages, private handles, RAII.
- Remove raw out-params and `*mut` handles from public APIs.
- Add filesystem/process/env integration tests, leak tests, and error tests.
- Acceptance: zero public raw pointers in Tier A APIs; `fs`/`process` tests
  pass on Linux and Windows (where available).

**P2.3 — `net`, `http`, `tls`.**
- Rewrite sockets/listeners/streams, HTTP body/headers/parser, and TLS
  client/server around the safe I/O traits; use the `Transport` enum, not a raw
  pointer/`dyn Read` default.
- Move raw TLS/mbedTLS and crypto primitives to `tls::raw`; add safe
  `Certificate`/`CertificateChain`/`PrivateKey`/`TlsConfig`/`TlsStream`, server
  accept, own certificate chain/key, `wrap(existing stream)`, ALPN/peer-
  certificate access, and one-shot crypto. `verify_hostname` is SAN-first.
- Add `AsyncRead`/`AsyncWrite` or explicitly park async I/O in `raw`; do not
  expose `*mut u8`/`'static` futures as safe async I/O.
- Add loopback HTTP/TLS integration tests (client and server), malformed input
  tests, and ASAN buffer tests.
- Acceptance: no public raw pointers in normal HTTP/TLS usage; TLS client/server
  handshake and HTTP request/response tests pass on TCC and LLVM.

**P2.4 — Data/media/crypto.**
- Rewrite `encoding`, `crypto`, `compression`, `archive`, `image`, `audio`,
  `font`, `regex`, `json`, `mime`, `datetime`, `uuid`, `osrand`, `cbor` to the
  safe policy.
- Add parser fuzz/negative tests for malformed inputs (JSON/CBOR, regex, image,
  archive, audio, font, HTTP headers/TLS records as applicable); round-trip
  tests for formats; allocation-failure tests. Add `--fuzz` to `scripts/test.sh`
  (fixed seed/crash corpus under `lang/tests/fuzz/corpus/`) and make it part of
  the Phase 2 release gate, not only Phase 3.
- Acceptance: zero public raw pointers in Tier A APIs; format tests and the
  fuzz corpus pass.

**P2.5 — Web/UI.**
- Rewrite `window`, `webview`, `page`, `components`, `router`, `server` around
  owned handles, the proven stored-callback form (`Callback`/witness), `&str`,
  `&[u8]`, `Result`.
- Add lifecycle tests, callback drop/unregister tests, a double-free/UAF test,
  SSR/escaping tests, and a real WebView smoke test where the environment
  supports it. `Window`/`WebView` are `!Send`; teardown quiesces in-flight
  callbacks before freeing.
- Acceptance: no public raw pointers in normal UI/web APIs; examples updated;
  callback lifecycle tests pass.

**P2.6 — Concurrency and async simplification.**
- Implement `thread::scope`, safe `spawn`, `Mutex`/`RwLock`/`Condvar`/channel
  safe APIs, `ThreadPool` safe submission.
- **Spike first:** compile the exact `for<'s> (&'s mut Scope<'s>) => void`
  form and the `ScopeData` erased-join representation before committing to
  removing `@scoped_callback`. If the checker cannot express it, fix the checker
  (safety-critical) or keep the annotation; do not ship a half-working `scope`.
- Replace `@scoped_callback`/witness public surface with `Scope`/`Fn` bounds
  once the spike passes; remove the compiler special case.
- Add safe `Pin`/`FutureHandle`/`spawn_send` tests; TSAN.
- Acceptance: no `@scoped_callback` needed for normal scoped work; TSAN passes;
  no public raw executor state.

**P2.7 — Test/compiler/plugin/build APIs.**
- Rewrite `test_env`/`test_report` to `&str` messages and safe handles.
- Add `compiler::api` and `lab` safe facades; keep raw CBI in `compiler::ffi`/
  `lab::raw`; update plugins.
- Acceptance: in-tree plugins compile against the facade; raw CBI is not
  required for normal macro code.

**P2.8a — Migration transformer and external-consumer inventory.**
- Inventory in-tree examples/apps, published components, docs snippets, and
  remote-import users; record them in `lang/docs/safety-sharpen/consumers.md`.
- Add a migration transformer for the mechanical removals (associated types,
  external inherent impls, `Box<[T]>`, slice/range patterns) where safe. It must
  not insert `unsafe`.
- Verify pinned remote dependencies either migrate or fail with `REMOVED-*`
  hints; record the policy in `BREAKING.md`.
- Acceptance: consumer inventory complete; transformer runs on the in-tree
  corpus; no unsafe insertion; migration guide covers every removed feature.

**P2.8 — API audit, docs, migration guide, release gate.**
- Generate public API snapshots for every Tier A library.
- Run `scripts/safety-sharpen-api-audit.sh`: zero public raw pointers/unsafe
  funcs outside Tier C plus the explicit reviewed allowlist.
- Update READMEs, examples, `BREAKING.md`, release notes, and `AGENTS.md`.
- If the release is still over 5,000,000 bytes after P1.7, finish the
  release-gating consolidation before tagging: complete the generic drop-path
  helper, move non-essential tooling out, compress strings/tables, and strip.
  Deeper size work remains P3.6.
- Add `scripts/safety-asan.sh` for the ASAN/leak gate: instrument the LLVM-native
  test executable (not the TCC JIT compiler), name the flags/build directory, and
  wire the lifecycle tests into CI. If ASAN cannot cover a target, record the
  substitute (valgrind/leak harness) explicitly.
- Run the full gate matrix on TCC and LLVM; ASAN/TSAN; no-panic sample.
- Tag the safe-API release.
- Acceptance: all Phase 2 exit criteria below.

#### Phase 2 exit criteria ("are the APIs ready to ship?")

The answer is **yes** only when all are true:

1. Tier A public API audit is clean: zero **ordinary** public raw-pointer
   parameters/returns, zero public `unsafe func` outside Tier C plus the
   explicit reviewed allowlist, zero public raw pointer fields in
   invariant-bearing handles. Explicit unsafe raw accessors (`span::data`,
   `Window::native_handle`, `environment::set_var`, etc.) are allowlisted and
   documented.
2. Every Tier A library has a positive safe API test, misuse/forgery test,
   drop test, and docs stating ownership/lifetime/failure/thread behavior.
3. `fs`, `net`, `http`, `tls`, `process`, `archive`, `image`, `audio`, `font`,
   `window`, `webview` integration tests pass on the claimed targets. Windows
   and macOS compile/tests are required for the libraries that claim them;
   Linux-only UI/webview backends are labeled as such. A target that cannot be
   run here must at least cross-compile and pass `--check`. Publish a
   per-library claimed-target matrix; a webview/window API may not ship
   “tested” on a platform where its only backend was never executed. The MIR
   interpreter is not a release gate for APIs using `dyn`, callbacks, async, or
   FFI; record interpreter exclusions explicitly rather than silently skipping.
4. A `[no_panic]` example module uses the explicitly annotated safe fallible
   core APIs (no dynamic dispatch/callback erasure) with zero reachable panics;
   allocation-failure tests pass. A per-Tier-A no-panic compatibility matrix
   (`safe`, `panics`, `untested`) is published. Full generic/async/interface
   effect propagation is P3.1, not a Phase 2 requirement; the release notes must
   not claim universal no-panic library support.
5. Send/Sync bounds and TSAN pass for concurrency/async; guards are `!Send`
   where required.
6. ASAN/leak checks pass for file, socket, TLS, image, archive, and window
   lifecycle tests.
7. TCC/LLVM main, safety, negative, removed, fuzz, api, async, libs, plugins,
   interpret, regexp, process, server, `MIRTests`, TSAN, audit all green;
   webview/universal run where available (or an environment exception is
   recorded); `--tls` ran once; `--check` cross-target compile-only passes;
   counts and baselines updated.
8. `BREAKING.md` and migration docs are complete; all in-tree users are updated.
9. Release `TCCCompiler` builds at **≤5,000,000 bytes** (4 MB goal); size/time
   metrics are recorded with per-job deltas. Over-ceiling blocks release until
   more consolidation lands; a temporary first-release exception requires an
   explicit user decision recorded in `STATUS.md` before tagging.
10. The API snapshots are committed and a named second reviewer (recorded in
    `STATUS.md`) has signed off.
11. Compile-time/RSS and generic-instantiation counts are recorded against the
    Phase 1 baseline (same SHA list, idle machine, median of 5, the small/
    std-heavy/tests modules named in §1.9); the safe API layers do not regress
    total compile time/RSS by more than 10–15% without a written justification
    and a fix plan. The second reviewer is a named person/agent recorded in
    `STATUS.md`, not an unspecified role.
12. Tier B audit: `compiler::api`/`lab` expose no public raw pointers/unsafe
    funcs outside `compiler::ffi`/`lab::raw`; every in-tree plugin builds against
    the facade and the plugin suite is green.
13. §1.5 measured goals are reported: Tier A `unsafe_blocks` reduced by at least
    50% versus the starting baseline (or a written justification per library),
    Tier A `panic_calls` limited to documented conveniences, and a no-panic
    trust matrix is attached. Missing a target blocks the release unless the
    user explicitly accepts the measured exception.

---

### Phase 3 — Harden and shrink (post-release)

**Goal:** close the remaining safety gaps, harden implementations, and reduce
compiler size/compile time toward 4–5 MB without changing the stable safe APIs
(except soundness fixes).

#### Jobs

**P3.1 — No-panic completion.**
- Implement `[no_alloc]`, `[no_unsafe]`, `[no_blocking]`, `[pure]`, module
  defaults, generic/async/interface propagation, and trap policy.
- Add negative/positive tests and a `no_panic` corpus.

**P3.2 — Fallible std/alloc implementation coverage.**
- Replace remaining panicking allocation/lock/option paths with fallible
  variants; add failing-allocator tests across containers, async, thread pool.
- Keep panicking conveniences but mark them non-`[no_panic]`.

**P3.3 — Move/borrow/init closure.**
- Close remaining HRTB source-to-stored/opaque/unsized cases, async type
  erasure, unsupported move shapes, and user-destructor temporaries.
- Prefer fail-closed rejection plus a safe API alternative; do not add new
  syntax without a concrete API need.
- Add regression tests for every closed gap.

**P3.4 — FFI/C ABI closure.**
- Complete target-aware ABI for claimed targets; reject unknown targets;
  variadic aggregates; callback ABI audits; `@stdcall`; bitfields; semantic
  wrapper audit.
- Add cross-target compile tests and FFI safety tests.

**P3.5 — Send/Sync and async hardening.**
- Remove conservative gaps where safe; audit executor/iterator raw state,
  views, `FutureHandle`, and unbound-`dyn` conservatism (keep `dyn`).
- Expand TSAN coverage.

**P3.6 — Compiler size/compile-time consolidation.**
- Replace per-dimensional array drop selectors with a generic runtime drop-path
  descriptor + helper; keep constant-index fast paths.
- Move `--safety-report`, help/docs, and non-essential tooling out of the core
  binary (plugin or compressed table).
- Share MIR/safety tables, compress strings, strip release binaries, use
  size-optimized flags where they do not hurt runtime.
- Target: ≤5 MB hard, 4 MB goal, with compile-time/RSS improvements measured.

**P3.7 — Implementation quality.**
- Add `// SAFETY:` comments to every remaining unsafe block.
- Expand the Phase 2 fuzz/corpus coverage into long-running targets for parsers,
  regex, image/audio/archive, JSON/CBOR; keep the Phase 2 fixed corpus as a
  regression gate.
- Add release checklist, docs, and API stability notes.
- Continue the release train: implementation improvements, no API churn.

---

## 6. Status tracking and reporting

### 6.1 Status file

Create `lang/docs/safety-sharpen/STATUS.md` with one row per job:

```markdown
| ID | Phase | Area | Job | Status | Owner | Evidence (commit / command / timestamp) |
|---|---|---|---|---|---|---|
| P1.1 | 1 | compiler | release build repair + baseline | [x] | agent | abc123; `cmake --build ...` + `./scripts/safety-sharpen-metrics.sh`; 2026-11-08T... |
| P1.2 | 1 | language | keep dyn/DynBox + dynamic-call tests | [~] | agent | `./scripts/test.sh --tcc --safety`; abc124; timestamp |
| P2.1a | 2 | std | IoError + Read/Write/Seek | [ ] | agent | pending |
```

Status values: `[ ]` not started, `[~]` in progress, `[x]` done, `[!]` blocked.
Canonical schema: every row has ID, phase, area, job, status, owner, and an
evidence field containing a commit SHA, the exact command(s) run, and a
timestamp. A job is done only when its evidence is present and the mapped gate
passed. `safety-sharpen-status.sh` flags stale evidence (missing timestamp,
uncommitted tree, or a gate command that no longer exists). The script maps each
job ID to its required suite/audit in `lang/docs/safety-sharpen/gates.json`.

### 6.2 Status script

Add `scripts/safety-sharpen-status.sh`:

- prints implemented/total and percentage per phase and overall;
- prints the next incomplete job;
- prints blocked jobs with the reason;
- exits nonzero if `STATUS.md` is malformed.

The user should be able to ask:

- "how much safety sharpen plan has been implemented?" → script output.
- "how much is remaining?" → script output.
- "continue implementation of safety sharpen plan" → the agent reads
  `STATUS.md`, runs the next phase/job, and does not stop until the phase is
  complete or blocked.
- "are the library APIs ready to ship?" → run the Phase 2 API audit + checklist
  and answer yes/no with the failing items.

### 6.3 Commit and review rules

- One logical commit per job or sub-job; commit messages name the job ID.
- Every commit is green on the relevant suites.
- Do not leave dead feature code behind; remove it in the same job.
- Do not add a workaround (`unsafe` in a safe API, a special-case annotation, a
  hidden raw pointer) to avoid fixing the root cause.
- If a safety-critical feature is missing, implement it; do not route around it.

---

## 7. Testing and gates

### 7.1 Existing gates to keep

- `./scripts/test.sh --tcc` / `--llvm`
- `--safety`, `--negative`, `--async`, `--libs`, `--plugins`, `--interpret`
- `--regexp`, `--process`, `--server`; `--webview`/`--universal` where the
  display/toolkit is available; `--tls` once before the Phase 2 release gate
- `--fuzz` (fixed parser/format corpus; added in P2.4) and `--removed`
  (removed-feature negatives; added in P1.3)
- `lang/tests/api/` safe API contract suite (added in P2.1; `--api` flag)
- `--check` cross-target compile-only gate for targets that cannot be run here
- `MIRTests`
- `./scripts/test.sh --llvm --thread-tsan`
- `./scripts/safety-asan.sh` (LLVM-native ASAN/leak gate; added in P2.8)
- `./scripts/safety-audit.sh`
- `./scripts/safety-baseline.sh`

### 7.2 New gates

- `scripts/safety-sharpen-metrics.sh` — size/time/RSS baseline.
- `scripts/safety-sharpen-api-audit.sh` — public API policy. It must inspect
  typed/reflection or `--safety-report`-style declaration data, not only grep
  text: type aliases, `any`, generic raw wrappers, CBI/macro-generated
  signatures, and `usize` handles must be visible. Emit a per-declaration
  snapshot and use `lang/docs/safety-sharpen/api-allowlist.txt` as data, not a
  hardcoded exception list.
- `scripts/safety-sharpen-status.sh` — plan status.
- `lang/tests/api/` — safe API contract suite (positive/misuse/forgery/drop).
- `lang/tests/no_panic/` — no-panic corpus (Phase 3).
- `lang/tests/removed/` — removed-feature negative tests.
- ASAN/leak suite for file/socket/TLS/image/archive/window lifecycle.
- Parser/format fuzz corpus: a small fixed seed/crash corpus runs in Phase 2
  (P2.4) for the parsers rewritten there; P3.7 expands coverage and adds
  long-running fuzz targets. The Phase 2 corpus path/command is
  `lang/tests/fuzz/corpus/` + `./scripts/test.sh --tcc --fuzz` (or the equivalent
  harness added in P2.4).

### 7.3 API audit allowlist

Tier C plus explicit allowlist (reviewed; each entry has a rationale):

- `cstd`
- `std::raw`, `core::raw`, `std::alloc` raw primitives, `std::mem` raw
  primitives, `std::maybe_uninit`/`manually_drop`/`non_null`/`pin` raw
  constructors
- specific inherently-unsafe primitives: `UnsafeCell::get_ref`,
  `Pin::get_unchecked_mut`/`from_raw`, `Waker::from_raw`,
  `FutureHandle`/`SendFutureHandle` raw frame/vtable access, `ManuallyDrop`
  `take`/`destroy`, `MaybeUninit::assume_init*`, `NonNull` unchecked
  constructors, allocator `dealloc`/`grow`/`shrink`, `u16_strlen`,
  raw thread spawn/join; these may stay public only under an `unsafe func`
  name and must have a safe wrapper for normal use
- explicit unsafe accessors with no safe equivalent: `span::data`,
  `Window::native_handle`, `WebView::native_handle`,
  `environment::set_var`/`remove_var`/`set_current_dir`
- `core::async` raw protocol
- `tls::raw`, `tls` cryptographic primitives
- `compiler::ffi`, `lab::raw`, `compiler_runtime`
- parser/CBI internals (`*_parser`, `*_cbi`, `*_ide`)
- `window::raw`, `webview::raw` platform bindings
- `atomic::raw`

Everything else must be safe. The audit allowlist is data in
`lang/docs/safety-sharpen/api-allowlist.txt`, not a hardcoded grep exception;
adding an entry requires a review note and a safe wrapper for the normal path.

### 7.4 Test counts

After Phase 1/2, update all counts in:
`AGENTS.md`, `.agents/skills/safety/SKILL.md`,
`lang/docs/safety/current-status.md`, `lang/docs/safety-sharpen/STATUS.md`.

Do not report stale counts from the old safety plan.

---

## 8. Risks and mitigations

| Risk | Mitigation |
|---|---|
| Removing features breaks external code | Breaking release; `BREAKING.md` + migration errors; no silent aliases. |
| Safe API rewrite introduces unsoundness | Contract tests, `// SAFETY:` proofs, ASAN/TSAN, reviewer sign-off, fail-closed behavior. |
| Borrow checker cannot express `Scope`/callback APIs | Treat as a safety-critical language task; fix the checker, do not add `unsafe` workarounds. |
| Compiler size does not reach 5 MB | Track per-job; consolidate array drop selectors; move tooling out; strip; report deltas. |
| Compile time regresses from generic APIs | Benchmark; prefer `&[u8]`/references, `@inline` only small wrappers, avoid monomorphization explosions. |
| Safe API performance regresses | Add I/O/TLS/HTTP/JSON/image benchmarks to the release gate; >5% core regression requires fix/justification. |
| TLS/crypto security bug in the safe wrapper | SAN-first hostname verification, constant-time audits, malformed-record corpus, crypto review checklist. |
| Callback trampoline UAF/double-free | Exactly-once free, unregister-on-drop, quiesce in-flight callbacks, ASAN/leak tests, `!Send` UI handles. |
| One huge breaking release slips | Phase gates, measured status, rollback tag before Phase 1; cut the release at the Phase 2 boundary even if Phase 3 is unfinished. |
| Removing associated types affects tests/docs | Delete or convert to generic parameters; update docs and keyword lists. Keep `dyn`/tuple structs/unit tuple. |
| Release build remains broken | P1.1 is the first job; no size work proceeds until it builds. |
| C ABI gaps on claimed targets | Keep `SAFE-FFI-*` rejection; complete target classifier; cross-target tests. |
| Interpreter divergence on new APIs | Keep core interpreter suites green; publish explicit exclusions for dyn/callback/async/FFI APIs; compile/check on native. |
| CBI enum shift after feature removal | Same-commit C++/Chemical enum sync or reserve removed slots; plugin suite in the gate. |
| Scope creep | Feature policy + freeze list; no new syntax without a safe API. |

### 8.1 Rollback and external migration

- **Rollback tag:** before Phase 1 starts, tag the current commit
  `safety-sharpen-preflight`; every phase exit gets an annotated tag. If a phase
  cannot be completed, the branch can return to the last phase tag without
  losing the analysis.
- **External consumers:** inventory in-tree examples/apps (`lang/compiled/`),
  published components, docs snippets, and remote-import users; add a migration
  transformer for the mechanical removals (associated types, external inherent
  impls, `Box<[T]>`, slice patterns) where safe.
- **Pinned remote dependencies:** a module using removed syntax fails with a
  `REMOVED-*` diagnostic that includes the migration hint; no silent compatibility
  shim. If a pinned dependency cannot be migrated, the user must upgrade/patch
  it; record this in `BREAKING.md`.
- **No force-push / no history rewrite** after the preflight tag; rollback is a
  revert to the tag or a follow-up fix, so the safety audit history stays intact.

---

## 9. Release checklist (safe-API release)

Before tagging:

- [ ] Release `TCCCompiler` builds at **≤5,000,000 bytes** (4 MB goal); size/time/RSS recorded.
- [ ] Phase 2 API audit clean for all Tier A libraries.
- [ ] API snapshots committed; docs updated.
- [ ] `BREAKING.md` and migration guide complete.
- [ ] TCC/LLVM main, safety, negative, removed, fuzz, api, async, libs,
      plugins, interpret, regexp, process, server green; webview/universal where
      available; `--tls` once; `--check` cross-target compile-only passes.
- [ ] `MIRTests`, TSAN, ASAN/leak suite green.
- [ ] Claimed-target matrix, no-panic matrix, compile-time/RSS gate, Tier B
      audit, and §1.5 unsafe/panic reduction report are attached to the release
      notes.
- [ ] `[no_panic]` example and allocation-failure tests green; per-Tier-A
      no-panic compatibility matrix published.
- [ ] All in-tree examples/apps migrated; no removed syntax remains.
- [ ] Release notes state the compatibility break and the safety guarantees.
- [ ] A second reviewer signs the API audit.

After tagging, Phase 3 continues with implementation hardening and size
reduction; API changes after this point require a soundness justification and a
new breaking-release note.

---

## 10. Instructions to the implementing agent

1. Read this plan and `lang/docs/safety-sharpen/STATUS.md`.
2. Pick the first incomplete job in Phase 1, then Phase 2, then Phase 3.
3. Complete the job, run its gates, update `STATUS.md`, commit, and continue to
   the next job. Do not stop at a job boundary.
4. Stop only when the current phase exit criteria are met or a genuine blocker
   is recorded in `BLOCKED.md` with the exact command/output.
5. Do not add new syntax, sugar, or features unless a safe API cannot be
   expressed without them; if so, write the justification in `STATUS.md` first.
6. Do not use `unsafe` inside a safe API as a workaround; fix the root cause.
7. Keep both backends green and keep the interpreter green for the language
   surface it supports; no backend may read the AST. For APIs that use `dyn`,
   callbacks, async, or FFI, record an explicit interpreter exclusion (compile/
   check on native) rather than silently treating the interpreter as a gate.
8. Keep docs/skills in sync in the same commit as the code.
9. Do not use `-j 16` for builds; use the configured safe parallelism.
10. When asked "how much is implemented/remaining", run the status script and
    report exact counts, not prose estimates.
11. When asked "are the APIs ready to ship", run the API audit and the Phase 2
    checklist; answer yes only if every item passes.

---

## Appendix A — Commit/feature inventory used for this analysis

The analysis covered the safety commits from `5e0e8ed9c` through
`8bccc8d02`, including:

- safety core: diagnostics, audit, MIR move/init/drop, borrow/lifetime,
  unsafe/FFI, auto traits, async, no-panic first slice;
- feature expansion: `dyn` (`49870f6fb`…`48fd280fe`, **kept**), tuples
  (`f865f2568`, `eee9767dc`, **kept**), slices (`a2c6113df`…`8c22799a9`,
  **kept as views; `Box<[T]>` removed**), tuple structs/patterns
  (`f9d17a2b8`, `9a27fbb1e`, `781a05c64`, `0254d6f8a`, `3575bc1ee`,
  **tuple structs/unit tuple kept; nested/slice patterns removed**), associated
  types (`a69c74204`…`b0cbdef0c`, **removed**), external/generic inherent impls
  (`67d18402a`, `d515c63a0`, **removed**);
- FFI/auto-trait/stdlib audit tranches and move/init precision commits through
  `8bccc8d02`;
- docs/skills updates through safety status v5.80.

The feature additions are roughly: `dyn` 0.9k net lines, associated types 1.2k,
tuples 0.9k, tuple structs 0.3k, patterns 1.4k, slices 1.5k (including docs and
tests). These are conservative approximations, not size measurements. `dyn`,
tuple structs, and the unit tuple are kept; the removal-phase size win comes
from associated types, the nested/slice pattern machinery, `Box<[T]>`, and
external/generic inherent impls.

## Appendix B — Library audit table (source counts)

The table in §1.5 is a triage snapshot from the current tree. The execution
phase replaces it with generated, per-declaration snapshots. Counts are allowed
to change when a safe API replaces a raw one; the audit baseline must be updated
with reviewed justification, not silently.

## Appendix C — Non-goals

- Do not implement GATs, const generics, `impl Trait`, or derive. (Trait
  objects/`dyn` already exist and stay.)
- Do not add a JS/JVM/WASM backend in this plan.
- Do not implement GC.
- Do not add syntax sugar for arrays/tuples/patterns beyond the kept forms.
- Do not weaken any `SAFE-*` rule to make a library compile; fix the API or the
  checker.
