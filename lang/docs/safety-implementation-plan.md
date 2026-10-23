# Chemical Full Safety Parity Plan (v4.1)

> **Status:** In progress — S0.1 (guardrails) underway.
> **Supersedes:** v2/v3/v3.1/v3.2 (git history).
>
> ## Implementation status (2026-10-23)
>
> **Landed**
> - `Diag` optional stable `code`; coded output is `error[SAFE-*]:` /
>   `warn[SAFE-*]:`; uncoded diagnostics keep `error:`.
> - `Diag::with_code`, related-information rendering; `Diagnoser`/
>   `ASTDiagnoser` code plumbing.
> - `SAFE-INIT-001` on use of an uninitialized variable.
> - `SAFE-UNSAFE-001` on safe-context raw-pointer dereference (report-only
>   warning).
> - Negative harness accepts coded errors and has `expect_compile_warning_id`.
> - `lang/tests/safety/` module + `--safety` suite, wired through
>   `lang/tests/build.lab` and `scripts/test.sh`; negative output path is
>   backend-specific.
> - Runtime-failure harness (`expect_runtime_panic`) using
>   `process::execute` and compiler subprocesses; smoke + panic tests on TCC and
>   LLVM (`2/2` each).
> - Tests: `lang/tests/negative/src/safety_diagnostics.ch`; TCC main
>   `2318/2318`, LLVM main `2316/2316`, interpreter `1907/1907`, negative
>   `329/329`.
>
> - S0.2 `--safety-report` audit mode:
>   - text table and JSONL (`--safety-report json`), one object per module;
>   - categories: raw pointer types/derefs/addrs/casts, unsafe blocks/values,
>     extern declarations/calls, variadic calls, panic/abort, unreachable,
>     new/delete/dealloc/destruct, allocator/memory calls, inline asm, mutable/
>     extern globals;
>   - collected after type checking and before codegen, with per-module dedup;
>   - safety-suite test verifies the report contains pointer/deref counts.
>   - Known limitation: generic/master impl bodies are skipped (their evaluated
>     containers can be stale after instantiation); concrete declarations are
>     reported. This is tracked for S0.3 audit refinement.
>
> - S0.3 baseline/audit gate:
>   - `scripts/safety-audit.sh` checks `--safety-report` totals against
>     `lang/docs/safety/audit-baseline.json`; `--update` records a reviewed
>     baseline.
>   - `scripts/safety-baseline.sh` records compiler sizes and audit wall time in
>     `lang/docs/safety/perf-baseline.json`; fails on significant growth.
>   - The checked-in size baseline currently uses debug binaries; the release
>     build is refreshed after the release configuration is repaired (tracked as
>     a tooling item, not a safety-rule change).
>
> - S0.4 remove user opt-outs:
>   - removed `safety`, `safe_mode`, and `checks.*` from `LabModuleOptions`,
>     `ModuleOptionRegistry`, and `lang/libs/lab/src/lab.ch`;
>   - old `option safety = false` is now rejected as an unknown option;
>   - fixed module-file `option` parsing to consume the leading identifier;
>   - negative suite `330/330`.
>   - `unsafe(expr)`, `da_in_unsafe`, and `@allow_zeroed` remain until their
>     replacement stages (as planned).
>
> - S0.5 MIR/interpreter hardening (partial):
>   - MIR source interning is now O(log n) (`MIRBuilder::source_lookup_`)
>     instead of the previous linear scan;
>   - unknown interpreter opcodes are hard errors; `AtomicFence` is a
>     single-threaded no-op (comptime atomics remain correct for fences);
>   - `--mirtests` builds and runs `MIRTests` (`4524 checks`);
>   - interpreter provenance/bounds/UAF checks are deferred to M5.7, where they
>     are release-blocking before enforcement.
>
> - S0.6 `--check` lowering-only path:
>   - `--check` now runs MIR lowering and the structural verifier without
>     emitting C/LLVM or linking;
>   - verified with a good module (exit 0, no executable) and an uninitialized
>     read (exit 1, `SAFE-INIT-001`, no executable);
>   - full safety parity will be re-verified in M8 after M3–M6 passes land.
>
> **M0 complete. M1 started.**
>
> - S1.1a reference lifetime syntax: `&'a T` / `&'a mut T` parse into
>   `ReferenceType::lifetime`; representation and copying preserve it.
>   A positive compile test covers `&'a int`.
> - S1.1b lifetime parameter declarations: `func <'a, T> f(...)`,
>   `struct Foo<'a>`, and `impl<'a, T>` now collect lifetime parameters in
>   `BaseGenericDecl::lifetime_params`; duplicate lifetime names are rejected.
>   The Chemical grammar keeps generic parameters before the function name
>   (`func <'a> f`), unlike the reference language.
> - S1.1c generic lifetime arguments: `View<'a, T>` parses in type position;
>   lifetime arguments are stored on `GenericType::lifetime_args` and preserved
>   in copies/representation. A struct field `View<'a>` compiles. Generic
>   struct literals with explicit lifetime arguments (`View<'a> { ... }` and
>   `View<'a, T> { ... }`) now parse; expression-level generic argument lists
>   carry lifetime arguments to struct literals. Function-call and
>   `GenericInstIdentifier` lifetime arguments are still dropped (they are
>   erased, and elision/inference supplies the signature), so explicit
>   turbofish lifetimes are not represented on those nodes yet.
> - S1.0b lifetimes in generic impl targets: `impl<'a> Trait for View<'a>`
>   and `impl<'a, T> Trait for View<'a, T>` register and instantiate
>   end-to-end. `register_generic_impl` now accepts a lifetime-only parameter
>   list (it still requires a generic target type). Positive tests cover a
>   lifetime-only target and a mixed lifetime/type-parameter target, including
>   an explicit `View<'a, T> { ... }` literal and a method returning `&'a T`.
> - S1.0c impl-header where clauses: `impl<T> Trait for Wrapper<T> where T: Copy`
>   parses into `GenericImplDecl::where_clause`, is linked in the generic-param
>   scope, merged into the parameter's traits (so interface bits and bound
>   checks see it), and checked during demand-driven instantiation. A missing
>   bound is diagnosed as "does not satisfy bound ... required by this generic
>   implementation". This unlocks `impl<T> Copy for Wrapper<T> where T: Copy`
>   and similar conditional impls. Note: method bodies that call a bound method
>   on a generic field (`self.value.show()`) still hit the pre-existing generic
>   field-resolution limitation; inline `impl<T: Show>` has the same behavior.
> - S1.1d lifetime-name resolution (basic): `TypeVerifier` tracks declared
>   lifetimes from generic functions/structs/unions/variants/interfaces/impls
>   and legacy struct lifetimes; `'static` and `'self` are always valid;
>   unknown names are rejected (`unknown lifetime parameter`). Negative test
>   added.
> - S1.1e signature lifetime elision: `&self` methods tie elided outputs to
>   `'self`; one input lifetime supplies the output; one elided input gets the
>   internal `'elided` name; multiple distinct input lifetimes require an
>   explicit output lifetime. The no-input case stays permissive until the
>   borrow checker rejects the escape in M4. Positive and ambiguous tests added.
> - S1.1f where-clause lifetime bounds: `where 'a: 'b + 'c` parses into
>   `WhereClause::lifetime_bounds`; every referenced lifetime must be declared
>   (or `'static`/`'self`). Positive and unknown-bound tests added.
> - S1.1g type-parameter lifetime bounds: `where T: 'static` and
>   `where T: 'a + 'static` parse into `WhereConstraint::lifetime_bounds`
>   alongside trait constraints, and every referenced lifetime must be
>   declared. `&'static T` is accepted. Synthetic elided lifetimes now use the
>   lexer's no-apostrophe representation (`self`, `elided`) so all lifetime
>   names share one form. Positive and unknown-bound tests added.
> - Still pending in S1.1: enforcement of outlives constraints in the borrow
>   checker. Lifetimes are resolved/elided for declaration validity but not yet
>   related by region constraints.
> - S1.2 temporary lifetime extension is deferred to M4, where regions and
>   destruction points exist; it cannot be verified before the borrow checker.
> - S1.3a `std::PhantomData<T>`: added as a zero-sized marker and verified as a
>   field type (`std::PhantomData<&'a T>`) in a lifetime-generic struct,
>   including explicit `PhantomData<&'static int> {}` construction. The parser
>   now handles a lifetime inside a reference type in `isGenericEndAhead`, so
>   `View<&'a T>`-style generic arguments are recognized.
> - S1.3b variance analysis is deferred to the region/borrow-checker work in
>   M4; without regions there is no consumer for inferred variance. The
>   `PhantomData` representation it depends on is in place.
>
> - S1.4a opt-in `Copy` enforced: user-defined types no longer count as
>   `Copy` merely because they lack a destructor. Primitive types, pointers,
>   references, functions and arrays of `Copy` are built-in; user types opt in
>   with `impl Copy for T {}`. Destructor-bearing types are rejected. A
>   trivially copyable user type without an impl is a hard `SAFE-COPY-001`
>   error. The in-repo tree is migrated: `std::PhantomData<T>`,
>   `std::string_view`, `std::span<T>`, `JsProperty`, `JsParam`,
>   `ImportSpecifier` and `SsrText` opt in; `JsStateInit` uses `get_ptr`
>   instead of copying through `vector.get`. Positive and negative tests added.
> - S1.4b `Copy` impl validity: an `impl Copy` is rejected when the target has
>   a destructor (`SAFE-COPY-002`) or a field that is not Copy
>   (`SAFE-COPY-003`). Fields are checked recursively through arrays and nested
>   structs; a raw generic type parameter is rejected until impl-header
>   where-clauses can express `T: Copy`. The library suite is migrated
>   (`FontTable`, `RouteChainStep`, `UUID`) and green (719/719).
> - S1.4c destructor rules: `@delete` signatures are validated with
>   `SAFE-DROP-001`: exactly one implicit `self` reference (`&self` or
>   `&mut self`), no by-value `self`, and a void return. Generated destructors
>   are exempt. The existing symbol-resolution checks now carry the stable code
>   as well. Positive and negative tests added.
> - Still pending in S1.4: `Clone`/`Default` impl generation.
>
> Next: `Clone`/`Default` derive-style support, then the M1 ownership/move
> checker.
> - S0.3 baseline/size/performance gate + `scripts/safety-audit.sh`.
> - S0.4 remove module/CLI opt-outs.
>
> The detailed status is maintained in `.agents/skills/safety/SKILL.md`.

>
> **Goal:** the next release is fully safe for safe programs: ownership, moves,
> borrowing, lifetimes, initialization/validity, bounds/overflow/null, thread
> safety, async/Pin, and an explicit FFI/unsafe boundary. Safety is enforced for
> every module; there is no user-facing opt-out.
>
> **Parity contract:** a safe program in the reference language can be translated
> to Chemical (using Chemical syntax and the shipped standard library) with the
> same safety guarantees. API names and macro surface may differ; missing
> library APIs are tracked in the parity backlog and are release blockers for the
> programs in the parity corpus.
>
> **Reference semantics:** Rust-familiar ownership/borrowing. No public
> diagnostic or documentation mentions the reference language.
>
> **Companion documents:** [`chemical-safety-analysis.md`](chemical-safety-analysis.md),
> [`proposals/0004-borrow-checker-memory-safety.md`](proposals/0004-borrow-checker-memory-safety.md),
> [`proposals/0005-effect-system.md`](proposals/0005-effect-system.md),
> [`mir-design.md`](mir-design.md).

---

## 1. Locked decisions

| Area | Decision |
|---|---|
| Scope | Full safe-program parity, including ownership/borrowing/lifetimes, init/validity, bounds/overflow/null, Send/Sync/data races, FFI/unsafe, async/Pin, HRTB, dyn/object safety, associated types/GATs, const generics, tuples, `impl Trait`, derive support, UTF-8 `&str`, and a target-aware C ABI. |
| Release gating | No partial safe release. Intermediate commits are green, but the release gate is full language/API parity: every shipped API is implemented and safe, and the parity corpus (including all shipped APIs) passes. No suite is excluded from the gate. |
| Unsafe surface | Explicit `unsafe { }` blocks and `unsafe func`/`unsafe impl`. Safe code cannot perform unsafe operations implicitly. `unsafe` never disables ownership, initialization, or borrow checking. |
| Raw pointers | `*T`/`*mut T`: creation/copy/compare/null-check safe; dereference, projection, method call, indexing, arithmetic, allocation, deallocation require `unsafe`. |
| Borrow checking | Rust-faithful NLL + HRTB + variance + dropck, delivered in report→migrate→enforce stages. Unsupported patterns are rejected with a `SAFE-*` diagnostic, never silently accepted. |
| Runtime model | Bounds checks always on; integer overflow checked in debug and wrapping in release; division/shift always checked; failures panic. Checked/wrapping/saturating APIs available. Unwinding/`catch_unwind` supported for parity. |
| Standard library | Full safe surface for shipped libraries: `Box`, `Vec`, `String`/`&str`, `Rc`, `Arc`, `Cell`, `RefCell`, `Mutex`, `RwLock`, `Condvar`, atomics, collections, I/O, net/TLS/HTTP, async. `unsafe` confined to reviewed internals/FFI. |
| GC | Owned mode first. Native GC is a later mode; it relaxes lifetimes but keeps aliasing/race rules. |
| Opt-out | None. No module flag, CLI escape, or per-dependency exemption. |
| Enforcement style | **Report-only until the whole in-repo tree is migrated; then enforce per rule globally.** No error-suppressing allowlist. |
| Breaking changes | Allowed. Existing code is migrated in the same series. |
| Remote modules | Out of scope for this plan; owned remote modules (mongodb, etc.) are updated in a later compatibility pass. |
| LSP | Deferred. `--check` runs full MIR safety; LSP safety is a follow-up with its own latency/caching design. |
| Size | The 5 MB ceiling is a post-feature shrink target. This release tracks size but does not gate on it; a dedicated size-reduction milestone follows feature completion. |
| Performance | Compile-time and runtime budgets are tracked; safety metadata is compile-time only; runtime checks follow the Rust-compatible model. Optimize before release; no silent regressions. |

### 1.1 Generic impls

Top-level generic impls (`impl<T> Trait for Type<T>`) landed immediately before
this plan. Required follow-ups: `impl<'a, T>` lifetime parameters, where clauses
on impl headers, lifetime/const-aware instantiation, and tests on TCC/LLVM/
interpreter. These are prerequisites for `Drop`/`Deref`/`Index`/`Iterator`/
`Send`/`Sync` impls.

### 1.2 No allowlist

Earlier drafts used a compiler-internal allowlist that could suppress enforced
errors. It is removed. The process is:

1. **Report-only:** `SAFE-*` warnings; behavior unchanged. Report-only is a
   debug-build development mode, compiled out of release builds.
2. **Migrate:** rewrite every affected site. Warnings reach zero.
3. **Enforce:** flip the rule to an error. The enforce commit includes the
   migration diff and a CI assertion of zero report hits for the rule.

A rule is never called enforced while any violation remains. There is no
per-file, per-module, or per-dependency error suppression.

### 1.3 Release train

Because full parity is large, work is organized as milestones. **No milestone is
released alone.** The release branch is cut only when the full parity gate
passes. Intermediate commits are green but not shipped.

---

## 2. Definition of done

For full safe-program parity:

1. **Soundness.** No safe in-repo program can cause use-after-move, double-drop,
   use-after-free, dangling/null/misaligned/OOB references, uninitialized or
   invalid values, data races, UB through a correctly used safe FFI wrapper, or
   memory corruption through safe arithmetic/casts/indexing. Remote modules are
   governed by the later compatibility pass (§14.6).
2. **Completeness.** Every unsafe operation outside `unsafe` is a compile error.
   Every safe operation has a checked runtime path or a statically proven safe
   path.
3. **Precision.** The language/API parity corpus passes. Every known false
   positive is closed; every false negative is a release blocker.
4. **No opt-out.** No module/CLI/dependency escape; report-only is absent from
   release builds.
5. **Parity.** All language features in §5.0 are implemented and tested on TCC
   and LLVM; the interpreter runs every feature it supports, and unsupported
   coroutine/interface opcodes are hard diagnostics. Interpreter coverage gaps
   are release blockers for the interpreter corpus (M5.7/M8.1), not silent
   skips. All shipped-library APIs in the parity backlog are release-blocking;
   an unimplemented shipped API is a gate failure, not a backlog note.
6. **Performance.** Compile-time/RSS/runtime targets in §13 are met; size is
   tracked and handed to the post-feature shrink milestone.
7. **Portability.** TCC, LLVM, and the interpreter share the same MIR checks.
8. **Auditability.** Every remaining `unsafe` block is audited, justified, and
   tested.
9. **Diagnostics.** Stable `SAFE-*` IDs, notes/help, tests assert IDs.
10. **Tooling.** `--check` reports the same safety diagnostics as a full build;
    LSP safety is explicitly deferred.
11. **No-panic** is a separate approved effect system; it is not claimed here
    unless the effects milestone is included. The plan schedules it after full
    memory safety unless the release train includes it.

---

## 3. Safety contract

### 3.1 Safe API rule

A **safe API** is a function, method, type, or trait whose correct use cannot
cause UB in safe code. If callers must uphold a memory-safety invariant, the API
is `unsafe`.

Two mechanized rules back this up:

- **Construction privacy:** fields carrying invariants (`NonNull`, `Vec`,
  `String`, `FutureHandle`, `Waker`, `Layout`, `MutexGuard`) are private;
  aggregate init, pattern destructuring, and `@direct_init` cannot forge them
  from another module.
- **Reference provenance:** a safe function returning `&'a T`/`&'a mut T`
  derives the return region from a formal parameter region, from `'static`, or
  from `&self`/`&'a self`. A raw-pointer-backed field is allowed only when the
  access is tied to `&self`/`&'a self`; otherwise the function is `unsafe`.
  `MIRProvenanceCheck` enforces this for every safe return.

### 3.2 Invariants

| Invariant | Enforced by |
|---|---|
| Value initialized before read/move/borrow/drop | MIR move/init |
| Owned value moved at most once, dropped exactly once | ownership + per-path drop flags |
| Shared refs may alias; mutable ref exclusive | borrow checker |
| Reference never outlives referent | lifetimes + borrow checker |
| Reference non-null/aligned; slice length valid | construction + validity |
| Slice/array index in bounds | runtime check |
| Debug arithmetic no overflow; release wrapping defined | checked/wrapping lowering |
| No data race through safe APIs | `Send`/`Sync` + `&mut` exclusivity |
| FFI unsafe unless wrapper validates | unsafe checker + audit |
| `unsafe` code has stated invariants | API audit + docs |

### 3.3 What `unsafe` permits

- raw pointer deref/projection/method/index/arithmetic;
- calling `unsafe func`, `extern`, unsafe trait methods;
- allocation/deallocation and unchecked memory ops;
- union fields, `static mut`, extern globals;
- `transmute`, `assume_init`, `unreachable_unchecked`, inline asm;
- `unsafe impl`.

`unsafe` does **not** permit use-after-move, uninitialized reads, double-drop,
borrow-checker violations on references, or OOB safe indexing.

`zeroed<T>()` is **safe** when `T: Zeroable` (an **`unsafe trait`** with audited
impls); it is not in the unsafe-operation list. `new` returns an owning
`Box`-like handle and `delete` consumes that owner (so `delete p; delete p` is
not expressible in safe code); deleting a raw `*mut T` is `unsafe`. Raw
`malloc`/`dealloc`/`memcpy`/`transmute`/`asm` are unsafe. `unsafe trait` is a
required language feature (§5.3).

### 3.4 Panic/unwind model

- Recoverable failures use `Option`/`Result`.
- `panic` is safe. Default profile aborts; an unwinding profile supports
  `catch_unwind` and drop-on-unwind for parity.
- Bounds, debug overflow, division by zero, invalid shifts, failed
  `RefCell`/`Mutex` borrows panic.
- Panicking in `Drop` is defined: abort if already unwinding; otherwise unwind.
- No-panic effects (proposal 0005) are a separate milestone.

### 3.5 GC boundary

Owned and GC code do not mix implicitly. GC modules relax lifetime/UAF checks
but keep aliasing/race checks. GC is a later mode.

---

## 4. Current-state audit

### 4.1 What exists

| Mechanism | State | Notes |
|---|---|---|
| Destructors/RAII | Implemented | `@delete`, automatic member destructors, MIR `Drop`/`Destroy`, top-level drop flags. |
| Move tracking | Partial | Ad-hoc vectors in `SymResLinkBody`; not flow-sensitive. |
| `Copy` | "No destructor" rule | `where T: Copy` satisfied by non-destructible types; no opt-in, no `Drop` conflict. This is unsound for resource wrappers. |
| `Clone`/`Default`/`Delete` | Manual | Bounds only. |
| References | Pointer-shaped | `&T`/`&mut T`; lifetimes not attached to most view fields. |
| Lifetime syntax | Partial | Single `'a` on structs, `: 'self` return annotation. |
| `unsafe` | Weakly enforced | Deref warns; `@unsafe` check commented; `unsafe` disables DA; `unsafe "no_lifetime_check"` disables the only lifetime check. |
| FFI | `@extern` | Calls safe syntax; aggregate parameters use non-C ABI in MIR. |
| Runtime checks | None | Bounds/overflow/null/validity unchecked. |
| Thread safety | No model | Raw pthread/OS wrappers, atomics, `std::concurrent`; no `Send`/`Sync`. |
| Trait objects | Partial | Basic dynamic dispatch works; lifetimes/coercion of `dyn` unmodelled. |
| Slices/unsized | Absent | Arrays/raw pointers only. |
| Tuples/const generics/associated types/impl Trait/HRTB | Absent | Required for full parity. |
| `&str`/UTF-8 | Absent | `string`/`string_view` are byte containers. |
| Async | Shipped | Self-referential frames; no `Pin`; `FutureHandle<T>` erases borrows. |
| Derive macros | Absent | CBI macros exist; no built-in derive. |
| Macros/plugins | Generate AST/raw JS | Generated Chemical checked normally; raw JS outside native safety. |

### 4.2 Measured migration scale

Exact queries go in `lang/docs/safety/migration-metrics.md` (S0.2). Current
approximate counts:

| Item | Count |
|---|---|
| Raw-pointer type mentions in libs + tests | ~16,500 |
| Lines containing `unsafe` | ~2,900 |
| `unsafe(...)` expressions | ~2,500 |
| `malloc`/`realloc`/`calloc`/`free`/`dealloc`/`memcpy`/`memmove`/`memset` | ~265 libs; ~342 with tests |
| `new`/`dealloc`/`malloc`/`delete`/`destruct` | ~225/217/181/97/46 |
| `@extern` declarations | ~1,240 |
| `printf` calls | ~344 |
| Top-level `var` | ~165 |
| Inline `asm(` | ~91 |
| `zeroed:unsafe` | ~59 |
| CBI plugin code | ~15,400 lines; ~485 raw-pointer mentions in `lang/libs/compiler` |
| `build.lab` files | 13, ~1,318 lines, raw-pointer-heavy |

**Conclusion:** a language/runtime/std migration, not a warning flip.

### 4.3 Existing checks and fate

| Check | Where | Fate |
|---|---|---|
| Definite assignment | `TypeVerify` | Replaced by MIR move/init; remove `da_in_unsafe` at replacement. |
| Destructible deref/index | `TypeVerify`/`SymResLinkBody` | Replaced by borrow rules + raw-pointer unsafe; remove for `&mut String`/`&mut Vec`. |
| Temporary lifetime escape | `TypeVerify::VisitFunctionCall` | Replaced by lifetime inference. |
| Move tracking | `SymResLinkBody` | Replaced by MIR move/init. |
| `unsafe` deref warning | `SymResLinkBody` | Becomes error under unsafe checker. |
| `@unsafe` | `FunctionDeclaration` | Replaced by `unsafe func`; deprecated alias removed before release. |
| `unsafe "no_lifetime_check"` | `UnsafeBlock::flag_name` | Removed; named flags are not a semantic escape. |
| `@allow_zeroed`/`zeroed:unsafe` | Annotation/value | Replaced by `unsafe impl Zeroable`/`unsafe`. |
| Module `safety`/`safe_mode`/`checks.*` | `LabModuleOptions`, `ModuleOptionRegistry`, `lab.ch` | Removed; no user opt-out. |

### 4.4 Baseline

Pin the baseline SHA and commands in S0.3. Current tree after generic impls:
TCC `2318`, LLVM `2316`, interpreter `1907`, plugins `1305`, negative `327`.
Re-baseline explicitly; checked-in logs may lag.

### 4.5 Known soundness bugs to fix before enforcement

1. Raw-pointer-derived references/casts unchecked (`&*p`, `p as &mut T`,
   `0 as &T`).
2. Aggregate init/`@direct_init` does not enforce field privacy; `FutureHandle`,
   `Waker`, `Layout`, `NonNull` can be forged.
3. `vector::get_ref(&self) -> &mut T`.
4. `string_view`/`span`/`Chunk`/`dyn`/`std::function`/`FutureHandle` erase
   lifetimes.
5. Unchecked allocation arithmetic/OOM in vector/string.
6. `default_function_instance` stores a pointer into its own inline buffer and
   is moved by memcpy.
7. `unsafe` disables definite assignment; `unsafe(expr)`/`unsafe "flag"` are
   inconsistent.
8. `@extern` aggregate ABI is not the target C ABI.
9. Drop flags are AST heuristics, not per move path; partial moves/variants not
   tracked; `MIRMovePath` never built.
10. `MIRType` is module-interned; function-local regions must not go into it.
11. Bounds/overflow lowering does not fit one-terminator/single-result MIR.
12. C signed arithmetic and float→int casts are UB; release wrapping must be
    lowered explicitly.
13. Comptime/interpreter memory ops have no bounds/provenance checks.
14. `--check` stops before MIR, so it misses safety checks.
15. `vector::get` returns `T` for `Copy`; changing it breaks ~1,000 sites.

---

## 5. Target language (full parity)

### 5.0 Parity matrix

The release gate is full safe-program parity for the features below. "Implemented
and tested" means TCC, LLVM, and interpreter where applicable, plus negative and
positive tests.

| Area | Required features |
|---|---|
| Ownership | moves, opt-in `Copy`, `Clone`, `Drop`, partial moves, reinitialization, drop flags, `ManuallyDrop`, `MaybeUninit`, `mem::replace/take/swap/forget`, dropck |
| Borrowing | shared/mutable references, lifetime syntax/elision/inference, `'static`, variance, NLL, reborrowing, two-phase borrows, temporary lifetime extension, HRTB |
| Types | structs, tuple structs, tuples, variants/enums, unions (unsafe), arrays, slices, `str`, `Sized`/`?Sized`, `PhantomData`, `Pin`/`Unpin` |
| Traits | interfaces/traits, default methods, supertraits, associated types, GATs, auto traits, negative impls, `unsafe trait`, `dyn Trait`/object safety/upcasting, `impl Trait` (argument/return, RPITIT), where clauses, generic impls, arbitrary self types (`self: Pin<&mut Self>`, `self: Box<Self>`) |
| Core traits | `PartialEq`, `Eq`, `PartialOrd`, `Ord`, `Hash`/`Hasher`, `Display`, `Debug`, `From`/`Into`/`TryFrom`/`TryInto`, `Borrow`, `ToOwned`, `Error`, `FromIterator`/`Extend`, `DoubleEndedIterator`/`ExactSizeIterator`, `UnwindSafe`/`RefUnwindSafe`/`AssertUnwindSafe` |
| Generics | type/lifetime/const parameters, defaults, const generics (`[T; N]`), monomorphization |
| Closures | `Fn`/`FnMut`/`FnOnce`, capture modes, `move`, HRTB, `'static` bounds |
| Patterns/control | `if let`, `while let`, `let else`, `match` exhaustiveness, `?`/Try, ranges, `for`/`IntoIterator` |
| Strings | byte strings, `String`, `&str`, UTF-8 validation, `CString`/`CStr`, `OsStr`/`OsString` |
| Smart pointers | `Box`, `Rc`/`Weak`, `Arc`/`Weak`, `Cell`, `RefCell`, `UnsafeCell` |
| Collections | `Vec`, `VecDeque`, `String`, `HashMap`, `BTreeMap`, `HashSet`, `BTreeSet`, `BinaryHeap`, `LinkedList`, `Cow`, `OnceCell`, iterators/adapters |
| Threads | `Send`/`Sync`, `thread::spawn`, scoped threads, `JoinHandle`, `Mutex`, `RwLock`, `Condvar`, `Barrier`, `Once`, `OnceLock`, atomics, channels |
| Async | `Future`, `Pin`, `Unpin`, `.await`, executors, `Send` futures, cancellation, async closures, `async fn` in traits/RPITIT, arbitrary self types |
| FFI | `extern "C"`, target-aware C ABI, `repr(C/transparent/packed)`, callbacks, variadics, `static mut`, unions, `CString`/`CStr` |
| Unsafe | `unsafe` blocks/functions/impls/traits, raw pointers, casts/provenance, `transmute`, `assume_init`, `zeroed<T>()`, inline asm |
| Panic | `panic`, abort/unwind profiles, `catch_unwind`, `UnwindSafe`/`RefUnwindSafe`/`AssertUnwindSafe`, drop-on-unwind, no unwinding across FFI |
| Derive | `Copy`/`Clone`/`Default`/`PartialEq`/`Eq`/`Hash`/`Debug` equivalent generation |
| Const/comptime | const generics, const evaluation, comptime functions, hardened interpreter |
| Std API | shipped library surface in §11; parity backlog tracked per API |

**Not required for safety parity** (may differ or be provided by plugins/APIs):
literal Rust macro syntax (`println!`, `vec!`), crate/edition syntax, Rust
module syntax, and exact std function names. Equivalent Chemical APIs must exist
for the parity corpus.

### 5.1 Ownership and moves

- One owner unless `Copy`.
- `Copy` is **opt-in**: a type declares `impl Copy` (or a derive), and the
  compiler verifies all fields are `Copy` and there is no `Drop`. There is no
  implicit auto-`Copy` that could copy an OS handle.
- `&mut T: !Copy`; raw pointers `Copy`; `Box`/`Rc`/`Arc` not `Copy`.
- `Clone` explicit. `Drop` and `Copy` cannot coexist.
- Move makes a place uninitialized; full assignment reinitializes.
- Partial moves tracked per field/index path; parent becomes partial and drops
  field-wise.
- Partial moves out of `Drop` types rejected.
- Dynamic-index moves rejected; use `replace`/`Option::take`/`swap`.
- Unions: fields must be `ManuallyDrop` or the union is only used for C interop;
  dropping a union drops nothing. `Drop` fields in a union are rejected unless
  wrapped.
- Destructors run exactly once on every initialized path, reverse order.
  Abort profile: no drops on abort; unwinding profile: drops run during unwind.

### 5.2 Borrowing and lifetimes

```chemical
func <'a> longest(x : &'a str, y : &'a str) : &'a str { ... }

struct Parser<'a> {
    var input : &'a str
}

func <'a> parse(p : &mut Parser<'a>) : &'a str { ... }

impl<'a, T> Trait for Wrapper<'a, T> { ... }

func <'a, T, U, F> map(x : &'a T, f : F) : U
    where F : for<'b> Fn(&'b T) -> U { ... }
```

- `&T`/`&mut T`, optional `'a`.
- Elision: one input lifetime → all outputs; `&self`/`&mut self` supplies the
  output; otherwise error.
- `'static`, `T: 'a`, lifetime bounds, HRTB.
- Variance: `&T` covariant in `'a`/`T`; `&mut T` covariant in `'a`, invariant
  in `T`; `*T` covariant, `*mut T` invariant; `Cell`/`UnsafeCell` invariant;
  `PhantomData` participates (including `PhantomData<fn(T)>`/`PhantomData<*mut T>`
  forms).
- Reborrowing, two-phase borrows, temporary lifetime extension.
- Reference cannot outlive referent; mutable reference exclusive while live.
- `'static` for threads/static storage.
- `string_view`/`span`/`Chunk`/`std::function`/`FutureHandle` carry real
  lifetimes or are replaced by slices/owners.

### 5.3 Types and traits

- Associated types and GATs are required for full parity (`Deref<Target>`,
  `Iterator<Item>`, `Index<Output>`, `Try`).
- `impl Trait` in argument and return position.
- Tuples and tuple structs.
- Const generics and `[T; N]`.
- `dyn Trait` with object safety, auto-trait bounds (`dyn Trait + Send + Sync`),
  upcasting, and lifetime bounds.
- `unsafe trait` declarations (required for `Zeroable`, `Send`/`Sync` internals,
  allocator/pinning invariants).
- Arbitrary self types: `self: Pin<&mut Self>`, `self: Box<Self>`,
  `self: Rc<Self>`, `self: Arc<Self>`.
- Derive support for `Copy`, `Clone`, `Default`, `PartialEq`, `Eq`, `Hash`,
  `Debug` (compiler plugin or built-in).
- Core traits: `PartialEq`, `Eq`, `PartialOrd`, `Ord`, `Hash`/`Hasher`,
  `Display`, `Debug`, `From`/`Into`/`TryFrom`/`TryInto`, `Borrow`, `ToOwned`,
  `Error`, `FromIterator`/`Extend`, `DoubleEndedIterator`/`ExactSizeIterator`,
  `UnwindSafe`/`RefUnwindSafe`/`AssertUnwindSafe`.
- `Sized`/`?Sized`, `Unpin`, `Send`, `Sync`, `Copy`, `Drop`, `Clone`, `Default`.

### 5.4 Unsafe and FFI

```chemical
unsafe func raw_copy(dst : *mut u8, src : *mut u8, len : usize) { ... }

func safe_copy(dst : &mut [u8], src : &[u8]) {
    if dst.len() < src.len() { panic("destination too small") }
    unsafe { memcpy(dst.as_mut_ptr(), src.as_ptr(), src.len()) }
}
```

- `unsafe func` replaces `@unsafe`; `unsafe impl`.
- `unsafe func` bodies require explicit `unsafe` blocks.
- `@extern`/`extern "C"` calls unsafe; new code uses `extern "C"`.
- `static mut` access unsafe; immutable `static` requires `Sync`.
- Union fields and inline asm unsafe.
- `unsafe(expr)` removed or unified; `@allow_zeroed` replaced by `unsafe impl
  Zeroable`.
- Variadics (`any...`/C variadics) are an unsafe boundary. Safe printing is
  fully typed/compile-time-checked (no unchecked format-string mismatch, no
  borrowed `any` escaping the call). C `printf`-style variadics are `unsafe`
  and wrapped by typed APIs. `intrinsics::print` accepts only checked typed
  arguments; arbitrary `any` payloads require `unsafe`.

### 5.5 Cast/provenance matrix

| From → To | Safe? | Notes |
|---|---|---|
| numeric → numeric | Yes | Defined truncation; float→int defined, not C UB. |
| `*T` ↔ integer | Yes | Provenance loss allowed; deref unsafe. |
| `*T` → `*U`, `*mut T` ↔ `*T` | Yes | Raw-pointer casts. |
| `&T` → `*T`, `&mut T` → `*mut T` | Yes | Decay. |
| `&mut T` → `&T` | Yes | Reborrow. |
| `*T`/integer → `&T`/`&mut T` | **No** | Unsafe; includes `&*p`, `&mut *p`. |
| `&T` → `&mut T` | **No** | Unsafe. |
| `&T` → `&U` | Only interface/`dyn` upcast | Other reinterpret casts unsafe. |
| `&[T; N]` → `&[T]` | Yes | Unsize. |
| integer → `bool` | **No** | Checked constructor/unsafe. |
| integer → `enum` | **No** | Checked range/unsafe. |
| integer → variant tag | **No** | Only constructors/patterns. |
| raw pointer field/method/index | **No** | Unsafe projection. |
| fn ptr to `unsafe func`/`@extern` | Call unsafe | Unsafety at call. |
| `usize` → pointer → reference | **No** | Unsafe/provenance. |

### 5.6 Runtime checks and C semantics

| Operation | Debug | Release |
|---|---|---|
| Array/slice index | panic OOB | panic OOB |
| `get`/`get_mut` | `Option` | `Option` |
| `+`, `-`, `*`, unary `-` | panic overflow | wrap two's complement |
| `/`, `%` | panic zero; `MIN/-1` panic | same |
| `<<`, `>>` | panic shift ≥ bits | same |
| `as` numeric | defined | defined |
| float→int | defined/saturating | defined |
| raw deref | unsafe | unsafe |
| `transmute` size mismatch | compile error | compile error |

C backend must not rely on C signed-overflow UB or implementation-defined
float→int. Wrapping via unsigned arithmetic; explicit defined float→int. Test
TCC and LLVM.

### 5.7 Thread safety

- `Send`: move to another thread. `Sync`: `&T` shared.
- Auto-derived; negative impls; `unsafe impl` audited.
- Raw pointers `!Send + !Sync`; erased types carry explicit bounds.
- `&T: Send` iff `T: Sync`; `&mut T: Send` iff `T: Send`; `MutexGuard: !Send`.
- `Rc` `!Send + !Sync`; `RefCell<T>` is `Send` when `T: Send` and always
  `!Sync`; `Cell<T>` is `Send` when `T: Send` and always `!Sync`;
  `Arc<T>` `Send+Sync` for `T: Send+Sync`; `Mutex<T>` `Sync` for `T: Send`.
- `thread::spawn` requires `FnOnce + Send + 'static`; scoped threads support
  non-`'static` borrows with a scope guard.
- `static` requires `Sync`; `static mut` access unsafe.
- `UnsafeCell` only interior mutability primitive; wrappers encapsulate it.
- `@volatile` is not atomic; concurrent volatile access is a data race; volatile
  read/write unsafe.

### 5.8 Async/Pin

- `Future`, `Poll`, `Context`, `Waker`, `Pin`, `Unpin`, `.await`.
- Async closures, `async fn` in traits/RPITIT, arbitrary self types
  (`self: Pin<&mut Self>`), `IntoFuture`.
- Coroutine frame fields are stable places; borrows across `.await` are checked
  across the ramp/poll/drop split.
- `FutureHandle<T>` carries a lifetime and is `Send` only when the underlying
  future is `Send`.
- `Waker` construction from raw parts is unsafe; `Waker: Send + Sync`; safe
  `wake` requires a validated owned handle.
- `PhantomPinned`/`!Unpin` and Pin projection rules are required.
- `catch_unwind`/cancellation drop exactly once.

---

## 6. Compiler architecture

### 6.1 Pipeline and per-function checks

```
parse
  → declare/link signatures (lifetimes, generics, where)
  → generic instantiation
  → after-signature facts (auto traits/variance seeds, finalized lazily)
  → link bodies (unsafe context, lifetimes, borrows)
  → type verify (types, unsafe calls)
  → MIR lowering
  → MIRMoveCheck analysis (init/drop plan; sole state producer)
  → MIR elaboration (StorageLive/Dead, drop flags from the plan, checked blocks)
  → MIR move/init verify (plan vs inserted MIR)
  → MIR borrow/provenance check
  → MIR validity check
  → emit/interpret
```

**Every emitted/interpreted `MIRFunction` goes through `lower_and_check`:**
primary bodies, `async_poll`, `async_drop`, async `main` trampoline, lambda
functions, static-interface stubs, comptime functions, and generated
declarations. No caller may emit/interpret unchecked MIR. `--safety-report`
prints checked vs emitted/interpreted counts; a mismatch is a hard error.

**Fused placement.** The C path lowers/emits per function in a worker. Checks
run immediately after lowering and before emission, in the same worker,
worker-local arena. No module-wide MIR retention for the C path. Diagnostics are
collected per function and sorted at module end.

**`--check`** lowers MIR and runs safety passes without emission. **LSP is
deferred**; when added, it will use a background build with debounce/cancel/
cache and a latency budget, and must not report "unchecked" as success.

**Comptime** is a hardened trusted boundary: unsafe context is checked at
typecheck; the interpreter enforces allocation/provenance/bounds/uninitialized/
UAF at runtime; unsupported opcodes are hard diagnostics. All comptime bodies
must lower through the normal pipeline or be rejected. Borrow/move checking of
comptime bodies runs in that pipeline.

### 6.2 New modules

| Module | Responsibility |
|---|---|
| `compiler/safety/SafetyDiagnostics.{h,cpp}` | IDs, notes, help, spans. |
| `compiler/safety/UnsafeChecker.{h,cpp}` | Unsafe context, unsafe ops, FFI, unsafe impls. |
| `compiler/safety/AutoTraits.{h,cpp}` | `Copy`, `Send`, `Sync`, `Sized`, `Unpin`. |
| `compiler/safety/Variance.{h,cpp}` | Variance/subtyping. |
| `compiler/safety/LifetimeResolver.{h,cpp}` | Names, elision, `'static`, bounds, HRTB. |
| `compiler/mir/MIRMoveCheck.{h,cpp}` | Init/move/drop/path dataflow. |
| `compiler/mir/MIRBorrowCheck.{h,cpp}` | Regions, loans, NLL, two-phase, provenance. |
| `compiler/mir/MIRValidity.{h,cpp}` | Transmute/layout/union/FFI-decidable validity. |
| `compiler/mir/MIRRuntimeChecks.{h,cpp}` | Bounds/overflow lowering helpers. |

### 6.3 AST/type changes

- `Lifetime`, `LifetimeParam` (or `GenericTypeParameter` kind Type/Lifetime/Const).
- `ReferenceType` carries lifetime during resolution; MIR erases it.
- Function/struct/variant/union/interface/impl declarations carry lifetime and
  const parameter lists; replace single `string_view` lifetime and `: 'self`.
- Generic instantiation substitutes lifetimes/consts.
- Tuples, tuple structs, associated types/GATs, `impl Trait`, const generics.
- Auto-trait/variance/dropck facts are module-lifetime side tables keyed by
  lifetime-erased `MIRTypeId`; invalidated on new instantiations; lock discipline
  like `MIRTypeTable`.
- New `ASTNodeKind`/`BaseTypeKind`/`ValueKind`/`TokenType` appended and mirrored
  in `lang/libs/compiler/src/...` (CBI enum-sync rule).

### 6.4 MIR contract

**Types are lifetime-erased.** `MIRType` is module-interned/immutable; regions
are function-local. Do not add region IDs to `MIRTypeRecord`.

**Region signatures.** A module-lifetime `MIRRegionSignature` side table is built
from declarations before body lowering: region-parameter count, per-parameter
region, return region, declared outlives bounds, HRTB binders. `MIRBorrowCheck`
reads MIR + this table; no AST access. Region IDs are per-function records keyed
by `InstId`/`PlaceId`, not module `ConstantId`s. Build signatures before
parallel body lowering with explicit locking/merge.

**Ownership opcode contract** (refined):

| Opcode | Meaning | Types |
|---|---|---|
| `Load` | load a value; copying a non-`Copy` aggregate is a **verifier error**; lowering must use `Move` | scalar/`Copy` only |
| `Assign` | lifetime-aware overwrite/init; drops old value | any |
| `Store` | raw store; does not drop old value | initialization/unsafe paths |
| `Move`/`MoveInit` | move | non-Copy (Copy may copy) |
| `Copy`/`CopyInit` | bitwise copy, source remains | Copy |
| `MemCpy`/`MemSet` | raw bytes | unsafe/trusted |
| `Borrow`/`BorrowMut` | explicit borrow + region | references |
| `StorageLive`/`StorageDead` | storage lifetime | addressed/drop places |
| `FakeRead` | borrowck-only read | any |

**No `BoundsCheck`/`CheckedBinary`/`Panic` opcodes.** Lower bounds as
`Compare` + `CondBr` + `Call(__chc_panic)` + `Unreachable`. Lower debug overflow
as `Binary` + overflow `Compare` + `CondBr` + panic call. `checked_*`/
`overflowing_*` use `Compare`/`Select`. This preserves the one-terminator/
single-result MIR shape and avoids a new terminator.

**MIR elaboration (1b)** runs after `MIRMoveCheck` analysis and before
verification/borrow:
- `MIRMoveCheck` is the **sole producer** of init/move/drop state. It emits a
  per-block `MIRInitPlan` (states + required drop flags).
- Elaboration inserts StorageLive/Dead for addressed/drop places, drop flags
  from that plan, and checked-operation blocks. It does not invent drop state.
- A fast post-elaboration verify checks the inserted flags against the plan
  (no double-drop, no drop-of-uninitialized). Then borrow checking consumes the
  final MIR.
- `Assign` is the lifetime-aware init/overwrite opcode (drops old value);
  `Store` is only for known-uninitialized/raw/unsafe paths. `MIRMoveCheck`
  validates the choice.
- `MIRFacts`/`MIRSafetyFacts` computed once on final MIR and passed to checks and
  emission. `init_state` is a builder hint only.
- Source map at borrow/index/call/field sites; O(1) dedup; test >65k unique
  locations and a deterministic fallback.
- Build real move-path trees; use `MIR_INVALID_ID` (not 0) for "none".

**MIR opcode landing checklist.** For each new opcode (`Borrow`, `BorrowMut`,
`StorageLive`, `StorageDead`, `FakeRead`): update enum/flags/name/portability,
builder, lowerer, verifier, dump, C emitter, LLVM emitter, interpreter, facts,
and `MIRTests`; one green commit per opcode across all consumers.

**MIR verifier.** Mandatory safety passes are distinct from the optional
structural verifier. Wire `MIRTests`/`verify_module` into CI/`--assertions`; add
typed validation (operand types, block args, targets) for MIR consumed by safety
passes.

### 6.5 Diagnostics

- Add `Diag::code`; print `error[SAFE-*]: ...`.
- Implement related-info/help; update `core/diag/Diagnostic.*`, LSP
  `DiagnosticUtils.cpp`, and CBI `ASTDiagnoser` if exposed.
- Update negative harness to accept `error:` and `error[SAFE-`.
- Report mode prints one summary line per file; full detail behind
  `--safety-report=json`; never flood the 16 KB negative buffer.
- IDs: `SAFE-OWN-*`, `SAFE-BORROW-*`, `SAFE-LIFE-*`, `SAFE-INIT-*`,
  `SAFE-VALID-*`, `SAFE-BOUNDS-*`, `SAFE-OVERFLOW-*`, `SAFE-SEND-*`,
  `SAFE-UNSAFE-*`, `SAFE-FFI-*`, `SAFE-ASYNC-*`, `SAFE-DEP-*`.

---

## 7. Borrow checker design

### 7.1 Inputs

- MIR CFG/block args, places, borrows, storage live/dead.
- `MIRRegionSignature` side table (including HRTB binders).
- Move/init state from `MIRMoveCheck`.
- Type facts: `Copy`, `Drop`, `Sized`, variance, `Send`/`Sync`, lifetime
  bounds, auto traits.
- Source map.

### 7.2 NLL

1. Region variables for reference lifetimes/temporaries/HRTB.
2. Backward liveness; `Live(region, point)`.
3. Outlives constraints from subtyping/assignments/calls/returns/fields.
4. Loan constraints; loans live where region live.
5. Solve SCCs; propagate liveness through outlives edges.
6. Conflicts: overlapping loans with ≥1 mutable; move while loan live; drop of
   `Drop` type with live lifetime (dropck).
7. Two-phase: reserve before arg evaluation, activate at call.
8. Report first conflict with loan origins/help.

### 7.3 Place overlap

- `p` overlaps `p.field`/`p[i]`; siblings do not.
- **Dynamic indices overlap unless proven distinct.** `p[i]` and `p[j]` are
  overlapping when `i`/`j` are not compile-time constants or a proven disjoint
  range. Only constant indices can be disjoint. (`split_at_mut` uses unsafe
  internals.)
- Borrow keeps whole place live; disjoint field reborrows allowed.
- Whole-aggregate move conflicts with descendant loans; field move with
  field/parent.
- Raw-derived places unchecked; `unsafe` does not silence reference borrowck.

### 7.4 Lifetimes/variance/dropck

- Lifetimes resolved in signature linking; lowered to function regions.
- Elision/`'static`/`T: 'a`/HRTB.
- Variance per declaration from fields/`PhantomData`; raw pointer variance:
  `*T` covariant, `*mut T` invariant.
- `&mut T` invariant in `T`; `Cell`/`UnsafeCell` invariant.
- Dropck for `Drop` types; `Pin`/`Unpin` interactions.
- `PhantomData` ties lifetime/variance/auto traits.

### 7.5 Precision/gaps

- Unsupported patterns: `SAFE-BORROW-099: ... not yet supported; rewrite as ...`.
- `lang/docs/safety/borrow-gaps.md`; release gate requires empty for parity
  corpus.
- Hard cases: conditional returns, loop borrows, reborrows, two-phase, closures,
  HRTB, struct-stored refs, pattern moves, iterator invalidation, `'static`
  threads, dropck, temporary extension, async frame borrows, Pin projections.
- **Never accept a pattern the checker cannot prove safe.**

### 7.6 Performance

- One pass per function in existing parallel workers; no second pool.
- Sparse bitsets/arena; no per-node heap.
- Fast path only no-refs/no-drop/no-Copy; measure hit rate.
- Cache type facts by lifetime-erased `MIRTypeId`; finalize in
  `GenericInstantiator` hooks; invalidate on new instantiations.
- Source map only safety-relevant instructions.
- StorageLive/Dead only addressed/drop places.
- Track compile time/RSS per phase; optimize before release.

### 7.7 Async/Pin

Async is in scope:
- `Pin<P>`/`Unpin`, `PhantomPinned`, Pin projection.
- Coroutine-frame fields are stable places.
- Borrows across `.await` checked across ramp/poll/drop.
- `FutureHandle<T>` carries a lifetime and conditional `Send`.
- Cancellation/drop exactly once.

---

## 8. Initialization, validity, runtime checks

### 8.1 Initialization/moves

- MIR dataflow per move path: Uninitialized/Initialized/Moved/PartiallyMoved/
  Destroyed.
- Joins agree or drop flags; loops fixed point.
- Reads/moves/borrows/drops require initialization.
- Full assignment reinitializes; partial writes update children.
- Remove the "writing one member initializes the whole struct" heuristic.
- `MIRMoveCheck` produces the init/drop plan; elaboration inserts drop flags
  from it, per path, field-wise, variant-aware. No AST heuristic produces a
  flag.
- No partial moves out of `Drop`; no dynamic-index moves.
- Variants track active case; unions are always-initialized storage (unsafe
  access); union fields `ManuallyDrop` or C interop.
- **Init enforcement lands before move enforcement and before borrow
  enforcement** (same MIR dataflow). The borrow checker consumes initialized/
  moved state; enforcing moves before init would accept reads of uninitialized
  values. Stage order: init → move/drop → borrow (M3 before M4).

### 8.2 Validity

- `char` is a byte; `String`/`&str` are UTF-8; `string_view` is a byte view that
  validates at `&str` boundaries.
- `enum` integer-backed; validate declared range; `variant` tag in range and
  matches payload.
- `bool` 0/1.
- References non-null/aligned/live; slice length valid.
- `MaybeUninit<T>` only safe holder; `assume_init` unsafe.
- `zeroed<T>()` and `= zeroed` both require `T: Zeroable` (unsafe marker trait,
  audited impls). Integers/floats/char/bool/raw pointers/arrays/all-Zeroable
  aggregates blanket impl; enums/variants/references/NonNull/invariant types need
  explicit `unsafe impl`.
- `transmute` equal size/align, unsafe.
- `repr(packed)` fields cannot be borrowed; copy/unaligned unsafe.
- Unions unsafe field access.

### 8.3 Allocation/layout

- `Layout`: `size <= isize::MAX`, align power of two, `size*count` checked,
  always.
- OOM: `try_*` Result; infallible panics/aborts.
- `Vec`/`String` capacity arithmetic checked; no `data_cap * 2`/`+1` overflow.
- Over-aligned allocation for heap-indirected captures.
- `alloc(0)`/ZST/dealloc layout match tests.
- `make_with_capacity(SIZE_MAX)`, `resize(SIZE_MAX)` safe-call tests.

### 8.4 Bounds/overflow

- Indexing: Compare/CondBr/panic call/Unreachable/IndexAddr.
- `Index`/`IndexMut` checked; `get` Option; `get_unchecked` unsafe.
- Debug arithmetic: Binary/overflow Compare/CondBr/panic; release explicit
  wrapping.
- Div/mod zero, `MIN/-1`, shifts, unary negation checked.
- Casts defined; float→int defined.
- No C signed-overflow UB.

### 8.5 Null

- References non-null by construction; raw pointers may be null; deref unsafe.
- `NonNull<T>` safe wrapper; deref unsafe.
- FFI wrappers validate null before creating references.
- Debug null checks not a substitute for construction.

### 8.6 Panic/unwind/runtime ABI

- Every program, including bare module, links `__chc_panic`,
  `__chc_bounds_fail`, `__chc_overflow_fail` with source/message.
- Default abort profile; unwinding profile supports `catch_unwind` and
  drop-on-unwind.
- Panic in `Drop`: abort if already unwinding; otherwise unwind.
- No unwinding across FFI.
- `std::panic` convenience wrapper.

### 8.7 Interpreter/comptime

This is scheduled **before any enforcement** (S0.5), because comptime runs during
compilation.
- Allocation/provenance table; bounds/UAF/uninit checks in debug/CI.
- Unknown opcodes hard diagnostics.
- Safety corpus through interpreter as oracle.
- Comptime unsafe context at typecheck; runtime checks; fuzz; no compiler crash.
- Coroutine/interface opcodes must be implemented for interpreter parity or
  rejected as hard diagnostics; unsupported opcodes are release blockers for the
  interpreter corpus, not silent skips.

---

## 9. Thread safety

### 9.1 Auto traits

- `Send`/`Sync` auto; negative impls; `PhantomData`.
- Raw pointers `!Send + !Sync`.
- `&T: Send` iff `T: Sync`; `&mut T: Send` iff `T: Send`.
- `UnsafeCell: !Sync`; `Cell`/`RefCell: !Sync` (but `Send` when `T: Send`);
  `Rc: !Send + !Sync`.
- `MutexGuard`/`RwLockGuard: !Send`.
- `unsafe impl` audited.

### 9.2 Std audit

Before `Send`/`Sync` enforcement, audit types storing raw pointers:
`string`, `vector`, `Box`, `Arc`, `FutureHandle`, `Waker`, function pointers.
Add audited `unsafe impl Send/Sync` only where the ownership invariant proves it.
Negative impls for `Rc`, `Cell`, `RefCell`, guards. `Future<T> where T: Send`.
`Waker` is `Send + Sync`; construction from raw parts is `unsafe`; safe `wake`
requires a validated owned/refcounted handle (no raw vtable deref in a safe
method).

### 9.3 Erased types

`std::function`, `dyn`, `FutureHandle`, `Waker`, `Context`, thread handles carry
explicit `Send`/`Sync` bounds. A safe closure capturing `Rc` must not become
`Send` through erasure. `FutureHandle<T>: Send` only when the underlying future
is `Send`.

### 9.4 Primitives

- `Arc`, `Mutex`, `RwLock`, `Condvar`, `Barrier`, `Once`, `OnceLock`.
- Atomics safe methods; `AtomicPtr` deref unsafe.
- `thread::spawn`/`JoinHandle`; scoped threads.
- `std::concurrent::Future<T>`/`threadpool.ch` audited/replaced.

### 9.5 Data-race tests

Compile-fail: `Rc` across threads, non-`Send` closure, non-`'static` capture,
`&mut` shared, top-level `var`/`static mut`. Runtime TSAN on `Arc<Mutex>`, scoped
threads, channels/executor.

---

## 10. Unsafe and FFI

### 10.1 Unsafe checker

- Unsafe ops inside `unsafe` context.
- `unsafe func` bodies require explicit `unsafe` blocks.
- `unsafe impl`.
- Raw deref/projection/method/index/arith.
- Allocation/deallocation, raw mem ops, `transmute`/`assume_init`/`asm`/union/
  `static mut`/extern globals/variadics.
- `placement new`/`dealloc`/`destruct` unsafe. `new` returns an owning
  `Box`-like handle and `delete` consumes it; deleting a raw `*mut T` is
  `unsafe`. Double-delete is not expressible in safe code.
- Cast matrix §5.5.
- Fn pointers to unsafe/extern unsafe to call.
- `unsafe` does not disable DA/borrow/init; remove `da_in_unsafe`, named flags,
  `unsafe(expr)` bypasses at replacement time.

### 10.2 Construction/privacy

- Enforce field privacy in aggregate init/pattern/`@direct_init`.
- Invariant types private fields + checked/unsafe constructors: `NonNull`, `Vec`,
  `String`, `FutureHandle`, `Waker`, `Layout`, `MutexGuard`.
- `std::function::make2` and raw constructors internal/unsafe.
- `@allow_zeroed` -> `unsafe impl Zeroable`.
- Forge tests cross-module/generic/CBI.
- Plugin-generated `unsafe` is a hard error at the macro invocation unless the
  plugin calls an audited unsafe function; no allowlist.

### 10.3 Self-referential values

- v1: heap-indirect all inline captures (`default_function_instance`) with
  aligned allocation.
- Async/Pin: `Pin`/`Unpin`/`PhantomPinned` for futures.
- User-written self-referential safe types are rejected with `SAFE-VALID-*`
  unless heap-indirect or pinned.
- Move test checks internal pointer.

### 10.4 FFI

- Extern calls unsafe.
- `repr(C)`/`repr(transparent)`/`repr(packed)` for ABI types; `@extern struct` ≡
  `repr(C)` during migration.
- **Full target-aware C ABI**: by-value aggregates/returns, unions, bitfields
  (if supported), callbacks, variadics. `MIRABI` becomes target-aware (SysV/
  Win64/AArch64); no legacy prototype ABI for extern.
- `repr` layout static assertions.
- Variadic default promotions (`char`→`int`, `float`→`double`); char signedness;
  bool representation; reject unsupported long double/128-bit.
- Callbacks extern "C"; no unwinding; variadics unsafe with typed wrappers.
- Safe wrappers validate null/alignment/length/lifetime/ownership/error codes/
  thread safety/reentrancy; small `unsafe` blocks.
- `CString`/`CStr`, `OsStr`/`OsString`; UTF-8 at boundaries.

### 10.5 Trusted core/plugins

- Unsafe allowed in core/cstd/std internals/platform wrappers/audited FFI.
- Applications may use `unsafe`; safe apps must not need it.
- CBI plugins may not forge `unsafe`/unchecked AST; generated Chemical checked
  normally; diagnostics point at macro invocation.
- Enum-sync rule for new AST/type/token values.
- CI counts unsafe per module; unreviewed growth fails.

---

## 11. Standard library rebuild

### 11.1 Bootstrap DAG (acyclic)

```
core types: Option/Result/panic (or minimal core equivalents; std re-exports)
NonNull / MaybeUninit / ManuallyDrop / PhantomData / PhantomPinned
Drop/Copy/Clone/Default semantics
trait definitions: Deref/DerefMut/Index/IndexMut/Iterator/IntoIterator/Fn
UnsafeCell
Box<T>
slices / fat pointers
Box<[T]>
Vec<T> (raw internals)
String (UTF-8) / &str / string_view
core_atomic (no std import; rename atomic or create module)
Arc / Mutex / RwLock / Condvar / Barrier / Once (requires repr/extern)
Cell -> Rc / RefCell
core Iterator/IntoIterator (needs Option/Result)
closures/Fn traits
iterator adapters
collections
OS / protocol / web libraries
async executor/channels
```

- `core` imports nothing; `Option`/`Result`/`panic` must be in core or a
  dependency-free core module.
- `atomic` currently imports `std`; create `core_atomic` or remove the import.
- `UnsafeCell` precedes `Cell`/`RefCell`/`Mutex`.
- `Mutex`/`Condvar` need `repr`/`extern "C"` before safe.
- `Box<T>` before slices; `Box<[T]>` after slices.
- `Deref` trait definitions before Box; impls after Box.
- `String` is UTF-8; `string_view` validates at `&str` boundaries.

### 11.2 API migration table

| Old | New | Notes |
|---|---|---|
| `vector::get(i): T where T: Copy` | `get(i): Option<&T>` / `get_mut` | ~1,000 sites; keep `at` for Copy. |
| `vector::get_ptr` | `as_mut_ptr` (unsafe deref) | 218 sites. |
| `vector::data(): *T` | `as_slice()/as_ptr()` | ~1,600 `.data()` sites; `Linear<T>` migration. |
| `vector::set_len` | unsafe `set_len` | 1 site. |
| `vector::push` | safe push (moves) | internal MaybeUninit/ptr::write. |
| `string::data()` | `as_bytes()/as_ptr()` | UTF-8. |
| `string_view` | `&str`/`&[u8]` | raw constructors unsafe. |
| `span<T>` | `&[T]`/`&mut [T]` | remove ad-hoc view. |
| `std::function` | closure/Fn traits | lifetimes/auto traits. |
| `FutureHandle<T>` | `Pin<Box<dyn Future + Send + 'a>>` or owned handle | no erased borrows. |
| `alloc_bytes`/`dealloc_bytes` | `Box`/allocator API | raw allocation unsafe. |

### 11.3 API rules

- Public APIs safe unless caller invariants; then unsafe.
- `get` references; by-value only Copy.
- `set_len`/`from_raw_parts`/`get_unchecked`/`assume_init`/`transmute` unsafe.
- `Drop` only automatic cleanup; close/flush explicit for OS resources.
- Iteration via Iterator/IntoIterator; `for` desugars.
- `Result` for I/O; panic for invariant violations.

### 11.4 Naming

Primary names: `Box`, `Vec`, `String`, `Rc`, `Arc`, `Cell`, `RefCell`, `Mutex`,
`RwLock`, `Option`, `Result`; `vector`/`string` deprecated aliases during
migration, removed before release.

### 11.5 Parity backlog

`lang/docs/safety/parity-backlog.md` lists shipped APIs and gaps. Release blockers
are the programs in the parity corpus; the backlog is public and prioritized.
No shipped API may be safe but unsound.

---

## 12. Testing strategy

### 12.1 Layers and wiring

| Layer | Location | Wiring |
|---|---|---|
| Compile-fail | `lang/tests/negative/src/safety_*.ch` | `--negative`; `expect_compile_error_id`; TCC+LLVM. |
| Compile-pass/runtime | `lang/tests/common/src/safety_*.ch` | `--tcc`, `--llvm`, `--interpret` via `run_safety_tests()`. |
| Runtime-fail | `lang/tests/safety/runtime/` | `--safety`; expected panic. |
| Parity | `lang/tests/safety/parity/` | `--safety`; manifest. |
| Library migration | existing suites | all green. |
| MIR unit | `MIRTests` | `--mirtests`/CI. |
| Sanitizers | CI | LLVM-only ASAN/UBSAN; selected TSAN. |
| Fuzz | `lang/tests/safety/fuzz/` | nightly. |

**Exact wiring:**

- `lang/tests/safety/chemical.mod`, `src/main.ch`, `runtime/`, `parity/`.
- `lang/tests/build.lab`: `test_safety_exe(ctx)`; `ctx.has_arg("test-safety")`.
- `scripts/test.sh`: `TEST_SAFETY`; `--safety`; `--arg-test-safety`; output
  `safety-tests-$backend.exe`; add to `--all`; `parse_suite_counts` requires
  `Summary: N tests - P passed, F failed`.
- `lang/tests/src/tests.ch` (compiled main), `lang/tests/interpret/src/main.ch`,
  and native-common call `run_safety_tests()`.
- `--mirtests` builds/runs `MIRTests` separately; not in `--all` until it prints
  the summary shape.
- Negative output per backend: `negative-tests-{tcc,llvm}.exe`.
- `Diag::code` + `expect_compile_error_id`; accept `error:` and `error[SAFE-`.
- `expect_runtime_panic` on `process::spawn/wait` using
  `status.signaled`/`status.signal`/`status.code`, timeout, stderr capture.
- LSP `DiagnosticUtils.cpp` carries code/related info when LSP safety is added.
- `scripts/safety-audit.sh` and `--safety-report` diff command.

### 12.2 Matrix

| Test | TCC | LLVM | Interpreter |
|---|---|---|---|
| Compile-fail | Yes | Yes | No |
| Compile-pass/runtime | Yes | Yes | Yes, capability-gated |
| Runtime-fail | Yes | Yes | Gated |
| Sanitizers | No | ASAN/UBSAN/TSAN | No |
| MIR unit | Yes | Yes | Yes |

### 12.3 Per-feature contract

Every safety commit:
1. Negative test (reject, ID, no executable).
2. Positive test (compile/run).
3. Unsafe-context test (legal unsafe; other checks still fire).
4. Migration test (libs compile; counts zero for the rule).
5. Backend test.
6. Report→migrate→enforce commit trail.

### 12.4 Parity corpus

- Categories: ownership, borrow, lifetimes, HRTB, init/validity, runtime,
  threads, unsafe/FFI, std, async, dyn, const generics, tuples, closures,
  UTF-8.
- Outcome (`accept`/`reject` + ID) + expected output/exit.
- Manifest: `lang/tests/safety/parity/manifest.tsv`, one row per case:
  `case<TAB>kind<TAB>expected_id<TAB>expected_exit<TAB>expected_stdout<TAB>timeout_ms<TAB>backends<TAB>dependencies`.
  `kind` is `accept` or `reject`; `expected_id` is empty for accept; `backends`
  is a comma list (`tcc,llvm,interp`).
- Harness: accept cases compile+run and compare stdout/exit; reject cases invoke
  the compiler, assert nonzero exit, `error[SAFE-*]`, no executable, and the
  expected ID. Reject cases run with the release configuration (no report-only).
- Independent corpus ported from well-known programs; reference compiler used as
  a development oracle, not a build dependency.
- Release gate: all safe cases accepted, all unsafe rejected, zero open gaps for
  the parity corpus.

### 12.5 No false positives/negatives

- Unsupported safe pattern -> compile error with rewrite.
- Every rule has a negative test.
- Fuzz/property + sanitizers.
- User-reported false positive is a release blocker.

### 12.6 Additional tests

- Cast/provenance, `&*p`, pointer field/method/index, int→bool/enum/variant.
- Borrowed/erased types, async borrows, HRTB, dyn, tuples/const generics.
- Construction/privacy forge tests.
- Self-referential moves; aligned capture allocation.
- `repr(packed)` unaligned refs.
- Allocation arithmetic/OOM.
- Release wrapping/bounds/float→int on TCC/LLVM.
- FFI full ABI by-value/callbacks/variadics/layout.
- Comptime/interpreter OOB/UAF/uninit; unsupported opcodes hard error.
- Runtime-fail tests run under both abort and unwinding profiles; expected exit
  code/signal differs and is part of the manifest.
- Determinism across workers/seed.
- `--check` parity; LSP deferred but tracked.
- Plugin-generated unsafe attribution.
- Unsafe audit.
- Safe canary: Vec/String/iterators/threads no user unsafe.

### 12.7 Gates

**Path→suite map** (maintained in `scripts/test.sh` or a helper):

| Changed paths | Required suites |
|---|---|
| `compiler/mir/`, `compiler/safety/`, `parser/`, `compiler/symres/`, `compiler/typeverify/`, `compiler/generics/`, `ast/` | TCC main + interpret + negative + LLVM main + `MIRTests` |
| `compiler/CodegenMir.cpp`, `preprocess/`, C/LLVM emitters | TCC + LLVM main + `MIRTests` |
| `lang/libs/core`, `lang/libs/std`, `lang/libs/cstd` | TCC + LLVM main/libs + interpret |
| `lang/libs/*` other | that library suite + TCC main |
| `lang/libs/*_cbi`, compiler plugins | `--plugins` + TCC main |
| tests only | affected suite |

**Per-commit:**
- build `TCCCompiler`, `Compiler`, `MIRTests`;
- run the path→suite map above (always TCC main + negative; LLVM/interpret when
  required);
- changed sub-suite (`--libs`, `--plugins`, `--interpret`, `--safety`);
- `MIRTests` via `--mirtests` (separate; prints its own OK line);
- `--safety-report` diff against the checked-in baseline (zero for enforced
  rules);
- `--check` smoke test for a known-bad and known-good program when the change
  touches safety passes.

**PR/merge:**
- TCC + LLVM main/interpret/negative/plugins/libs/async/process/regexp;
- `--safety`, `MIRTests`, `--check`, sanitizer subset;
- parity accept/reject harness.

**Nightly:** `--all --include-tls`, webview/universal, fuzz, benchmarks.

**CI wiring:** update `.github/workflows/compile-tests.yml` (or a new
`safety.yml`) with per-commit and PR jobs; add `MIRTests`, `--safety`,
`--check`, sanitizer, and benchmark jobs. Release artifacts:
`safety-tests-{tcc,llvm}` and the parity manifest report.

**Release:** §19.

`--mirtests` builds `MIRTests` and requires exit 0; it is not part of `--all`
until it prints `Summary: N tests - P passed, F failed`. `--check` is invoked as
`chemical <module> --check` and must exit nonzero with `SAFE-*` for a known-bad
program and zero for a known-good one.

**Test budget:** parallel compile-fail/parity; `--safety` separate; keep `--tcc`
fast; triage old negatives with acceptable IDs; do not rely on one substring.

---

## 13. Performance and size

### 13.1 Performance targets

Pin baseline SHA, idle machine, median of 5. Update `scripts/mir-baseline.sh`:
- include `safety`;
- run `-bm-modules`/`-bm-files` and parse phase timings;
- record SHA, dirty state, target triple, mode, compiler size;
- measure release `TCCCompiler`;
- emit per-phase JSON; CI reports growth.

Targets (not hard release gate until measured):
- compile time/RSS: no more than 10–15% total over the pre-safety baseline;
- borrow check: ≤5% wall / ≤3% RSS on std-heavy code;
- runtime bounds: ≤5% on the bounds benchmark; optimize loops/cold paths;
- TCC (no optimizer): measure separately; hoist checks or use audited unchecked
  std paths.
- If targets are missed, optimize before release; no silent regression.

### 13.2 TCCCompiler size

The release binary is already ~5,196,920 bytes, over the 5 MB steady-state
ceiling. Per the agreed scope, size is **tracked but not gated for this release**.
A **post-release** shrink milestone (M9) targets 5 MB:
- move `--safety-report`, diagnostics formatting, corpus metadata, and other
  non-essential tooling out of the core binary;
- compress tables/strings;
- share MIR/safety tables;
- profile binary sections.
If 5 MB is not achievable, document a signed exception with a dated reduction
plan. No safety check may be dropped to hit size.

### 13.3 Runtime

- Bounds checks required; optimize loop ranges/hoisting/cold paths.
- TCC has no optimizer; measure bounds-heavy TCC; hoist MIR checks or audited
  unchecked std paths.
- Overflow debug-only; release wrapping explicit.
- No runtime borrow metadata.

### 13.4 Hot spots

- `MIRType` lifetime-erased; no type-table explosion.
- `MIRFacts`/`MIRSafetyFacts` once per function.
- Source map safety-relevant only; O(1) dedup.
- StorageLive/Dead only addressed/drop places.
- Fast path measured.
- One worker pipeline.
- Report/diagnostic formatting off hot path.
- `--check` latency budget.

---

## 14. Migration strategy

### 14.1 Report → migrate → enforce

1. **Report-only:** `SAFE-*` warnings; no behavior change; debug-build only.
2. **Migrate:** wrap/rewrite every affected site; warnings zero for the rule.
3. **Enforce:** error; include migration diff; CI asserts zero report hits.

No allowlist. A rule is enforced only when the whole in-repo tree is clean.
Runtime rules (bounds/overflow/ABI/drop order) use **observe/count** first, then
migration, then enable.

### 14.2 Order

1. Infrastructure/diagnostics/test wiring/interpreter hardening.
2. Type-system prerequisites: lifetimes, Drop/Copy, slices, Box, traits,
   associated types/GATs, const generics, tuples, impl Trait, closures/HRTB,
   dyn, UTF-8.
3. CBI/lab/build.lab/bootstrap migration.
4. Unsafe boundary: report → migrate → enforce per rule.
5. Safe primitives and container/view APIs.
6. Init + ownership: report → migrate → enforce (init before move/drop).
7. Borrow/lifetimes/async: report → migrate → enforce.
8. Validity/runtime.
9. Send/Sync/threads.
10. OS/protocol/web/stdlib.
11. Parity/perf/release; size reduction is post-release (M9).

### 14.3 Tools

- `--safety-report=json`: rule, file, function, count, category; summary line
  per file.
- Suggestions include `unsafe { ... }` wrappers for mechanical cases.
- One-off transformer may apply wrappers; every block reviewed; no automatic
  unsafe insertion into safe APIs.
- `scripts/safety-audit.sh` counts unsafe per module; fails on growth.
- `lang/docs/safety/migration-metrics.md`.

### 14.4 CBI/lab/bootstrap

Before raw-pointer projection/method enforcement:
- `build.lab` entry is `*mut BuildContext`/`*mut LabJob`/`*mut Module`; add an
  unsafe adapter in `lang/libs/lab` or a compiler-recognized safe entry
  (`BuildContextRef<'a>`, `JobRef<'a>`, `ModuleRef<'a>`). Decide in S2.0a.
- Migrate `lang/libs/lab`, 13 `build.lab`, `lang/libs/compiler`, all `*_cbi`,
  `test`, and test harness. Split S2.0 into a/b/c/d/e with counts.
- Plugin-generated `unsafe` is a hard error unless it calls an audited unsafe
  function; diagnostics point at macro invocation.
- Enum-sync updates for new AST/type/token values.

### 14.5 Diagnostic compatibility

- New errors can preempt old expected errors. Before removing an old check,
  update negative tests to accept a set of IDs/phases; avoid double diagnostics.
- Keep temporary-lifetime diagnostic until lifetime inference replaces it.
- Remove old move tracking only when MIR move/init is sole source.
- Remove deprecated aliases (`@unsafe`, `unsafe(expr)`, `@allow_zeroed`,
  `zeroed:unsafe`, `vector`/`string` aliases) before release; add a CI gate.

### 14.6 Remote dependencies

Out of scope for this plan. In-repo modules must comply; owned remote modules
are updated in a later compatibility pass. No per-dependency opt-out exists for
in-repo builds. `SAFE-DEP-*` is reserved for the future remote pass.

---

## 15. Milestones and commit plan

Each enforcement feature is report→migrate→enforce; runtime rules are
observe→migrate→enable. Each commit is green under §12.7. Sizes and timing are
indicative; full parity is a large program (see §15.9).

### M0 — Guardrails (2–3 weeks)

| ID | Feature | Exit |
|---|---|---|
| S0.1 | Diagnostics/test wiring | `Diag::code`, `SAFE-*`, helpers, `--safety`, safety module, per-backend negative. |
| S0.2 | Audit report | JSON/text summary, metrics doc. |
| S0.3 | Baseline/gate | SHA, counts, compile/RSS/size, per-commit/PR/nightly commands; `scripts/safety-audit.sh`; checked-in `--safety-report` baseline JSON + diff command. |
| S0.4 | Remove user opt-outs | Module/CLI safety/checks removed; old semantics kept until replacements. |
| S0.5 | MIR source/verifier/interpreter hardening | O(1) source, verifier in CI, interpreter provenance/bounds early. |
| S0.6 | `--check` wiring | Lowering-only path + synthetic `SAFE-*` diagnostic; parity re-verified in M8. LSP deferred. |

### M1 — Type-system foundations (3–4 months)

| ID | Feature | Exit |
|---|---|---|
| S1.0a | Generic impl where-clauses | Tests. |
| S1.1 | Lifetime syntax/AST | Parse/symres. |
| S1.0b | Lifetimes in generic impls | Tests. |
| S1.2 | Elision/static/bounds/temp extension | Corpus. |
| S1.3 | Variance/PhantomData/PhantomPinned | Soundness. |
| S1.4 | Opt-in Copy/Drop/Clone/Default/unsafe trait | Drop/negative/unsafe-trait tests. |
| S1.5 | Auto traits/negative impls/UnwindSafe | Tests. |
| S1.6 | NonNull/MaybeUninit/ManuallyDrop/UnsafeCell | Unit/drop. |
| S1.7 | Trait definitions (Deref/Index/Iterator/Fn) | Trait tests. |
| S1.8 | Box<T> | Box tests. |
| S1.9 | Slices/fat pointers/Box<[T]> | Cross-backend. |
| S1.10 | Vec/byte String | Unit/drop. |
| S1.11 | UTF-8 String/&str/string_view validation | String tests. |
| S1.12 | Associated types/GATs + core traits (`PartialEq`/`Eq`/`PartialOrd`/`Ord`/`Hash`/`Display`/`Debug`/`From`/`Into`/`TryFrom`/`Error`) | Generic/error tests; derive prerequisites. |
| S1.13 | Tuples/tuple structs | Type/pattern tests. |
| S1.14 | Const generics/arrays `[T; N]` | Generic tests. |
| S1.15 | impl Trait (arg/return/RPITIT) + Box/Rc/Arc self types (Pin self types in S4.1) | Tests. |
| S1.16 | Closures/Fn traits/HRTB/async closures | Closure/HRTB. |
| S1.17 | ?/Try/if let/while let/let else | Error/pattern. |
| S1.18 | Derive support (needs core traits from S1.12) | Derive tests. |
| S1.19 | dyn/object safety/auto-trait objects | Trait-object. |
| S1.20 | repr/extern/static/static mut/top-level var migration | ABI/static. |

### M2 — Unsafe/FFI/C ABI (2 months)

| ID | Feature | Exit |
|---|---|---|
| S2.0a | build.lab entry ABI | Safe adapter/entry. |
| S2.0b | lab/build.lab migration | Zero raw projections. |
| S2.0c | compiler/CBI migration | Counts zero. |
| S2.0d | plugins/test harness | Counts zero. |
| S2.1 | Unsafe report | Warnings; keep `unsafe(expr)`/`da_in_unsafe` until M3/M5 replacements. |
| S2.2 | Unsafe migration → enforce raw-pointer/memory rules | Tree clean; negative tests; extern-call enforcement moves to S2.6 after C ABI; defer DA/`unsafe(expr)` to M3/M5. |
| S2.3 | Syntactic cast matrix (no provenance) | Matrix tests; provenance is S4.8. |
| S2.4 | Raw mem ops | Tests. |
| S2.5 | Full C ABI | Target-aware `MIRABI`; real C callee. |
| S2.6 | FFI wrappers/variadics/callbacks + extern-call unsafe enforcement | FFI tests. |
| S2.7 | Platform unsafe wrapping | Per-domain clean. |
| S2.8 | Tests/apps | All green. |
| S2.9 | Unsafe audit | CI counts. |

### M3 — Init/ownership/moves/drops (2 months)

| ID | Feature | Exit |
|---|---|---|
| S3.1 | MIR move paths, `MIRInitPlan`, drop elaboration from plan | MIR tests; no double-drop/drop-uninit. |
| S3.2 | Copy/move/use-after-move/double-drop report | Report. |
| S3.3 | Control-flow/partial moves/variants/unions report | Report. |
| S3.4 | Drop semantics/dropck prerequisites report | Report. |
| S3.5 | Init + move/drop migration → enforce (single dataflow) | Green; init before move. |
| S3.6 | Remove old DA/move tracking; `unsafe(expr)`/`da_in_unsafe` cleanup | Single source; warnings zero. |

### M4 — Borrow/lifetimes/async (3–4 months)

| ID | Feature | Exit |
|---|---|---|
| S4.1 | Pin/Unpin/PhantomPinned/async frames/Waker ownership | Async tests; Waker Send+Sync/unsafe construction. |
| S4.2 | Region signatures/HRTB (from declarations, before body lowering) | Tests. |
| S4.3 | MIR borrow lowering/autoref | MIR golden. |
| S4.4 | NLL core | Corpus. |
| S4.5 | Aliasing/moves/reborrows/dynamic-index overlap | Corpus. |
| S4.6 | Control flow/escape/dropck/variance | Corpus. |
| S4.7 | Two-phase | `v.push(v.len())`. |
| S4.8 | Provenance | Provenance tests. |
| S4.9 | Migration | Tree clean. |
| S4.10 | Enforce | Green. |

### M5 — Validity/runtime (2 months)

| ID | Feature | Exit |
|---|---|---|
| S5.1 | Validity (init is enforced in M3) | Tests. |
| S5.2 | Remaining validity/FFI checks | Tests. |
| S5.3 | Allocation/layout | Safe-call tests. |
| S5.4 | Bounds enforcement | OOB tests. |
| S5.5 | Overflow/C semantics | Tests. |
| S5.6 | Panic/unwind/catch_unwind + UnwindSafe/RefUnwindSafe/AssertUnwindSafe | Runtime tests, both profiles. |
| S5.7 | Interpreter/comptime final hardening | No crash. |

### M6 — Send/Sync/threads (1–2 months)

| ID | Feature | Exit |
|---|---|---|
| S6.0 | Std auto-trait audit | Tests. |
| S6.1 | Send/Sync enforcement | Compile-fail/positive. |
| S6.2 | Sync primitives | TSAN. |
| S6.3 | Threads/scoped threads | Thread tests. |
| S6.4 | Interior mutability | Negative/panic. |
| S6.5 | Erased/async Send | Async/net. |

### M7 — Stdlib full surface (3–4 months)

| ID | Feature | Exit |
|---|---|---|
| S7.1 | core/std audit | Report/sanitizers. |
| S7.2 | Collections/iterators/adapters (core traits are S1.12) | Tests. |
| S7.3 | fs/path/process/environment | Tests. |
| S7.4 | net/http/tls | Suites. |
| S7.5 | json/archive/image/audio/font/webview | Suites. |
| S7.6 | async/executor/channels | Async. |
| S7.7 | page/html/css/js/universal | Plugin/universal. |
| S7.8 | Tests/apps | All green. |
| S7.9 | Parity backlog closure | Corpus blockers. |

### M8 — Release hardening (1–2 months)

| ID | Feature | Exit |
|---|---|---|
| S8.1 | Full parity corpus + independent cases | Green. |
| S8.2 | Fuzz/property/differential | No crashes/unsound. |
| S8.3 | Performance optimization | Targets met. |
| S8.4 | Docs/error catalog/migration guide | Reviewed. |
| S8.5 | Unsafe audit/release checklist | Signed. |

### M9 — Post-release size/performance reduction (1–2 months, not release-gating)

| ID | Feature | Exit |
|---|---|---|
| S9.1 | Move diagnostics/report/corpus tooling out of core | Smaller binary. |
| S9.2 | Compress/share safety tables and strings | 5 MB target. |
| S9.3 | Profile binary sections | Documented result/exception. |

### 15.9 Schedule reality

Full parity (including async/Pin, HRTB, dyn, GATs, const generics, tuples,
`impl Trait`, full C ABI, UTF-8, and std migration) is a **large program**:
- ~90 stage items; with report/migrate/enforce triads and per-opcode/per-backend
  commits, roughly **300–450 commits**.
- One strong AI-augmented engineer: **18–24 months**.
- Two parallel streams (compiler semantics + stdlib/migration): **12–16 months**.
- The critical path is M1 (type system) and M4 (borrow/async); M7 is parallelizable.
- No milestone is released alone; the release branch is cut at M8; M9 is
  post-release size/performance reduction.

These are estimates, not commitments. The project proceeds feature by feature,
with every commit green and a review at each milestone boundary.

---

## 16. Dependency graph

```
M0 guardrails
  ├─ diagnostics/tests
  ├─ audit
  ├─ baseline/gate
  ├─ remove user opt-outs
  ├─ MIR source/verifier/interpreter hardening
  └─ --check safety (LSP deferred)

M1 type system
  ├─ S1.0a generic impl where-clauses
  ├─ S1.1 lifetimes → S1.0b impl lifetimes → S1.2 → S1.3
  ├─ S1.4 opt-in Copy/Drop → S1.5 auto traits
  ├─ S1.6 NonNull/MaybeUninit/UnsafeCell
  ├─ S1.7 trait defs → S1.8 Box → S1.9 slices → S1.10 Vec/String
  ├─ S1.11 UTF-8/&str
  ├─ S1.12 associated types/GATs
  ├─ S1.13 tuples
  ├─ S1.14 const generics
  ├─ S1.15 impl Trait
  ├─ S1.16 closures/HRTB
  ├─ S1.17 ?/patterns
  ├─ S1.18 derive
  ├─ S1.19 dyn/object safety
  └─ S1.20 repr/extern/static/top-level var

M2 build.lab/CBI migration → unsafe/FFI/C ABI
M3 init + ownership infrastructure/report → enforce (init before move/drop)
M4 Pin/async → borrow report → enforce
M5 validity/runtime
M6 Send/Sync/threads
M7 stdlib full surface
M8 release hardening (parity/perf/docs)
M9 post-release size reduction
```

Hard ordering:

- S1.1 before S1.0b.
- S1.3 (variance/PhantomData) before S1.6/S1.16.
- S1.4 before S1.5/S3.
- S1.6 before S1.8/S1.10; S1.8 before S1.9; S1.9 before S1.10.
- S1.7 trait definitions before S1.8 Box.
- S1.9/S1.10 before S1.12/S1.13/S1.14.
- S1.16 HRTB before borrowed callbacks/iterator adapters.
- S1.20 repr/extern before Mutex/Condvar/FFI.
- S2.0a–e before raw-pointer enforcement.
- S2.5 full C ABI before FFI wrappers.
- S3.1 before S3.5/S4.
- S3.5 init+move enforcement before S4 borrow enforcement.
- S4.1 Pin before S4.3/S4.10 async borrow enforcement.
- S4.2 region signatures before S4.3 body borrow lowering.
- S6.0 before S6.1.
- S6 before S7 async/threads.
- M7 before M8.

Parallel groups after M1 (respect the hard edges):
- **A1:** M3.1–M3.4 infrastructure/report (no enforcement).
- **A2:** M4.1–M4.8 borrow infrastructure/report (after M3.1; enforcement
  S4.10 after S3.5).
- **B:** M2/M5.4–M5.7 (runtime/interpreter) after M0.
- **C:** M1.10–M1.20 std/type surface (within M1, after its prerequisites).
- **D:** M7.2–M7.7 after M4/M6.
- **E:** M8 corpus/fuzz/perf after M7.

Only start a group after its hard prerequisites; do not parallelize dependent
stages.

---

## 17. Risk register

| Risk | Impact | Mitigation |
|---|---|---|
| Full parity schedule | Critical | Milestones; no release until full gate; 18–24 month estimate; parallel streams. |
| Borrowck false positives | High | Report→migrate→enforce; API rewrite before enforcement; gap list; provenance rule. |
| Borrowck false negatives | Critical | Per-rule negatives; differential/fuzz; verifier; review. |
| NLL/HRTB/Pin complexity | Critical | Dedicated stages; per-function checks; incremental corpus. |
| Auto-Copy unsoundness | Critical | Opt-in Copy; no implicit resource copy; Drop conflict; tests. |
| Enforcement unit | High | Per-rule global enforcement only when tree clean; no allowlist. |
| Raw-pointer/CBI migration | High | Safe handles; staged build.lab/CBI migration; counts. |
| Plugin forges unsafe | High | Hard error at invocation; audited unsafe function; enum sync. |
| C ABI mismatch | Critical | Target-aware MIRABI; real C callee tests. |
| C UB in release | Critical | Explicit wrapping/float-int; TCC/LLVM tests. |
| Allocation arithmetic/OOM | Critical | Always-checked Layout; safe-call tests. |
| Self-referential values | High | Heap indirection/Pin/PhantomPinned; reject unsafe forms. |
| Construction/privacy | Critical | Private fields/checked constructors; forge tests. |
| Send/Sync erasure | Critical | Raw ptr !Send/!Sync; explicit bounds; std audit; TSAN. |
| Async/Pin | High | Dedicated stage; frame borrows; conditional Send. |
| Interpreter/comptime UB | Critical | Early hardening; provenance/bounds; fuzz; oracle. |
| `--check` bypass | High | MIR safety in check; LSP deferred. |
| Type-system surface | High | Tuples/const generics/GATs/impl Trait/HRTB/dyn staged; parity corpus. |
| Std API surface | High | Parity backlog; release blockers listed; no silent unsound APIs. |
| TCC size | Medium | Track; post-feature shrink; no safety dropped. |
| Performance | High | Measure; optimize; TCC-specific; budgets. |
| Diagnostic compatibility | Medium | IDs; triage; one primary error. |
| Test suite time | Medium | Separate safety; parallel; per-commit vs PR gate. |
| Remote deps | Low | Out of scope; later pass. |
| GC/JS/macros | Medium | Owned/native first; document boundaries. |

---

## 18. Open decisions

All major scope decisions are now locked. Remaining implementation-level choices
are resolved in favor of the reference semantics unless the parity corpus forces
otherwise:

1. **Exact `Copy` syntax:** `impl Copy` vs derive; choose the one that reads best
   in Chemical and supports compile-time field checks.
2. **String representation:** UTF-8 `String`/`&str` with SSO or `Vec<u8>`; measure
   and keep the faster safe representation.
3. **C ABI implementation:** target-aware classifier vs compiler-generated
   adapter; choose per target, with real C callee tests.
4. **HRTB encoding:** parser/symres representation; must be usable in trait
   bounds and closures.
5. **GAT/impl Trait interaction:** stage order; both must be in place before
   generic iterator/borrow abstractions.
6. **Panic unwinding:** abort default; unwinding profile required for
   `catch_unwind` parity; implement both and test.
7. **Derive mechanism:** CBI plugin vs built-in; either is acceptable if
   diagnostics are good and compile time stays low.
8. **No-panic effects:** separate approved proposal; schedule after full memory
   safety unless the release train includes it.
9. **GC:** later mode; owned semantics first.
10. **Remote deps:** later compatibility pass.
11. **Size:** 5 MB target after feature completion; no safety dropped.
12. **Performance budgets:** validate against measured baselines; optimize
    before release.

---

## 19. Release checklist

- [ ] Full parity corpus green (language + std APIs + unsafe rejections).
- [ ] No user opt-out; report-only absent from release; zero report hits for
      every enforced rule.
- [ ] Every unsafe block audited/justified; plugin unsafe reviewed.
- [ ] Existing suites green: main, interpret, negative, plugins, libs, regexp,
      process, server, webview, async, universal/components. No suite is
      excluded from the release gate.
- [ ] `--tcc`, `--llvm`, `--tcc --interpret` green.
- [ ] `--safety`, `MIRTests`, sanitizer matrix green.
- [ ] Compile-time/RSS/runtime targets met; size tracked; shrink plan scheduled.
- [ ] `--check` safety parity verified; LSP deferred note.
- [ ] Full C ABI/FFI tests, allocation regressions, interpreter/comptime fuzz.
- [ ] std Send/Sync audit; threadpool Future audited; Waker safe.
- [ ] HRTB, dyn, GATs, const generics, tuples, impl Trait, UTF-8, async/Pin
      tested on all backends.
- [ ] Error catalog, safety guide, migration guide, parity backlog published.
- [ ] GC/macro/JS boundaries documented.
- [ ] No-panic explicitly not claimed unless the effects milestone is included.
- [ ] Release notes list breaking changes.

---

*Revision note: v4.1 follows the locked full-parity decision and the final
consistency review. It removes the safe-subset release, includes async/Pin, HRTB,
dyn, GATs, const generics, tuples, impl Trait, UTF-8, and full C ABI; makes
`Copy` opt-in; removes the error-suppressing allowlist; enforces init before
move/borrow; makes `new`/`delete` ownership-safe; closes the variadic `any`
printing hole; adds `unsafe trait`, `UnwindSafe`, and Waker ownership; adds the
missing core traits, collections, arbitrary self types, async closures/RPITIT,
and const-generic prerequisites; schedules interpreter hardening early; defers
LSP but keeps `--check`; treats 5 MB as a post-release shrink target (M9); and
keeps remote modules out of scope. The plan is a multi-milestone program
(~300–450 commits, 18–24 months one engineer, 12–16 with two streams), not a
single-drop change.*
