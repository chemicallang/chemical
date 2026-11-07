# Chemical Full Safety Parity Plan (plan v4.1; status v5.72)

> **Priority policy (2026-11-04): safety parity, not feature parity.** Chemical
> adopts Rust-like *safety semantics*, not Rust's feature surface. A remaining
> feature is release-blocking only when it is required to express safe code, to
> keep a safe API sound, or to meet a stated performance target. GATs, const
> generics, built-in derive, `impl Trait`/RPITIT, default-method parameter
> projections, match/switch-position patterns, `impl Trait for (T1, T2)`, raw
> unsized `[T]`, and full UTF-8 API parity are **deferred** unless a concrete
> safe stdlib API cannot be expressed without them. The critical path is now:
> stdlib/FFI wrapper soundness, move/borrow/init closure, async/concurrency,
> no-panic effects, and cross-target safety — not more syntax.

> **Status (2026-11-01):** M0 and M3 complete; M4/M5/M6 are complete for the
> currently enforceable safe surface, with the documented gaps in §4.5. M1
> feature expansion is frozen under the priority policy above; M2/M7 remain in
> progress; M8/M9 are pending. Direct async `FutureHandle` borrow
> escapes are checked in MIR, `SAFE-LIFE-005` rejects safe retaining APIs, the
> LLVM C ABI is target-aware for x86-64 System V, Windows x64/ARM64, i386
> SysV/Windows, ARM32 AAPCS (soft/hard-float) and AAPCS64 (unknown targets
> reject by-value `@extern` aggregates), `@scoped_callback` is
> body-checked, and async constructors check allocation failure.
> `FutureHandle<'a, T>` now carries a frame lifetime (`'static` for owning
> frames, elided for borrowed frames); call-site binding covers aggregate
> fields/pointer containers and custom-consumer local aliases, with the narrower
> gaps in §4.5 item 1. Non-`Copy` struct-field drops are field-tracked;
> unsupported move shapes are rejected with `SAFE-MOVE-004`.
> S5.6b (unwinding) is withdrawn by policy: panic always aborts; no unwind
> runtime, `catch_unwind`, or `UnwindSafe` traits are planned unless the C
> backend later makes unwinding practical.
> **Supersedes:** v2/v3/v3.1/v3.2 (git history).
>
> ## Implementation status (2026-10-29)
>
> **Historical log — kept for decision archaeology.** The entries below are a
> chronological record of the first implementation waves. Several “next”/“still
> pending” lines are now superseded. For current status use §4 (implemented
> surface and open gaps) and §15 (milestones); for the agent-facing summary use
> the `safety` skill.
>
> **Landed**
> - `Diag` optional stable `code`; coded output is `error[SAFE-*]:` /
>   `warn[SAFE-*]:`; uncoded diagnostics keep `error:`.
> - `Diag::with_code`, related-information rendering; `Diagnoser`/
>   `ASTDiagnoser` code plumbing.
> - `SAFE-INIT-001` on use of an uninitialized variable.
> - `SAFE-UNSAFE-001` on safe-context **raw-pointer** dereference (enforced;
>   reference dereferences are safe and not flagged) and `SAFE-PROV-001` on
>   raw-pointer-derived references/casts (enforced).
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
>   - Historical limitation (fixed 2026-10-31): impl bodies were skipped;
>     `SafetyReportVisitor` now walks them. Generic master/concrete impls can
>     still double-count syntax, so totals are conservative.
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
> - S1.2 temporary lifetime extension was deferred to M4, where regions and
>   destruction points exist; the initializer form is implemented there (see
>   the S4.2/S1.2 status below).
> - S1.3a `std::PhantomData<T>`: added as a zero-sized marker and verified as a
>   field type (`std::PhantomData<&'a T>`) in a lifetime-generic struct,
>   including explicit `PhantomData<&'static int> {}` construction. The parser
>   now handles a lifetime inside a reference type in `isGenericEndAhead`, so
>   `View<&'a T>`-style generic arguments are recognized.
> - S1.3b variance analysis moved to the region/borrow-checker work in M4 and is
>   implemented there as S4.2b; the `PhantomData` representation it depends on
>   is in place.
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
> - S1.4d `Default` satisfaction: `CoreNodes` marks `Default` and `Clone` with
>   interface bits; a `Default` bound is satisfied by primitive/non-container
>   types or by a user type with a zero-argument constructor (`@constructor` /
>   generated `make`). Generic-impl bound checks use the same rule. Positive
>   tests cover a user constructor and a primitive. `Clone` is marked for the
>   next derive step but is not auto-satisfied yet.
> - S1.4e `Clone` bounds: `Copy` implies `Clone` (a type with an `impl Copy`
>   satisfies a `Clone` bound without a separate `impl Clone`). A non-Copy type
>   still needs an explicit `impl Clone`; missing impls produce the normal
>   "does not satisfy where clause constraint 'T : Clone'". The rule is applied
>   in both normal where-clause checking and generic-impl bound checks.
>   Positive tests cover Copy-implies-Clone and an explicit Clone impl; a
>   negative test covers the missing impl.
> - S1.5a `Send`/`Sync` auto traits: `core::Send` and `core::Sync` are marker
>   interfaces with `SEND_BIT`/`SYNC_BIT`. Bounds are derived structurally from
>   fields: scalars are both; `*T`/`*mut T` are neither; `&T` is `Send` when
>   `T: Sync`, `&mut T` is `Send` when `T: Send`, and both are `Sync` when
>   `T: Sync`; arrays and nested fields recurse; `PhantomData<T>` participates
>   through its type arguments. Generic-impl bound checks use the same rules.
>   Tests cover positive structs, a raw-pointer field, a reference-to-pointer
>   field, and `PhantomData<*mut int>`.
> - S1.5b generic-function bound checking: generic parameters on a
>   `FunctionDeclaration` are parented to the declaration, not the wrapping
>   `GenericFuncDecl`, so the old bound checker skipped every generic-function
>   call. Explicit `f<T>(...)` now reads `call->generic_list`; inferred calls
>   recover the concrete type from argument types (through references,
>   pointers, arrays and generic arguments). This closes a real hole where
>   `f(r)` with `where T: Copy` accepted a non-Copy `r`. Positive and negative
>   tests cover both explicit and inferred calls.
> - S1.5c explicit auto-trait impls: `unsafe impl Send for T {}` /
>   `unsafe impl Sync for T {}` parse and override the structural derivation
>   (the author takes responsibility, as in the reference language). A plain
>   `impl Send/Sync` is rejected with `SAFE-AUTO-001`; explicit impls are
>   honored by both normal and generic-impl bound checks. `ImplDefinition`
>   preserves the `unsafe` flag through copies/instantiations.
> - S1.5d negative impls: `impl !Send for T {}` / `impl !Sync for T {}` parse
>   and are stored in a separate negative-impl set, so they never resolve as a
>   normal implementation. Negative impls win over structural derivation and
>   over an `unsafe impl`; conflicting positive/negative and duplicate negative
>   impls are rejected (`SAFE-AUTO-002`). They are currently restricted to
>   concrete `Send`/`Sync` targets; generic negative impls are rejected with a
>   clear diagnostic. Tests cover positive syntax, `!Send`/`!Sync` bound
>   rejection, non-auto-trait targets, and positive/negative conflicts.
> - S1.4f `Clone` implementations: std now implements `Clone` for all primitive
>   integer/float/bool/char types plus `string_view` and `span<T>`. Direct
>   `.clone()` calls on those types work; tests cover int, bool,
>   string_view and a deep `string` clone (`self.copy()`). Calling `.clone()`
>   on a generic `T: Clone` still hits the pre-existing generic-interface
>   method-resolution limitation (the same issue as bound methods on generic
>   fields), so that remains pending.
> - S5.3a container capacity overflow: `vector::reserve` rejects a capacity
>   whose byte size would overflow and `ensure_capacity_for_one_more` guards
>   the doubling; `string::move_data_to_heap`/`resize_heap` guard the trailing
>   NUL byte, `ensure_mut` guards growth, and `resize`/`append_with_len` guard
>   length arithmetic. Runtime safety tests cover `reserve(SIZE_MAX)` on both
>   containers and `string.resize(SIZE_MAX)`.
> - S5.3b allocation failure: `new T` checks the `malloc` result and aborts
>   before returning a null pointer; `std::box` does the same for its direct
>   allocation. (OOM cannot be forced in the ordinary test suite; the null check
>   is covered by inspection and the allocation-heavy suites staying green.)
> - S5.3c hash-map allocation/growth: `unordered_map` and `ordered_map` check
>   the bucket-array capacity arithmetic, reject a null table/node allocation,
>   and `ordered_map` seeds the first resize at 16 buckets (matching
>   `unordered_map`) instead of allocating zero buckets. Allocation-heavy suites
>   stay green; OOM itself is not force-testable.
> - S5.3h u16_string allocations: heap growth rejects `(capacity * 2) + 2`
>   overflow, null `malloc`/`realloc`, and length > capacity; UTF-8 append and
>   substring buffers receive the same checks.
> - S5.3g vector/thread-pool allocations: `vector::make_with_capacity` checks
>   byte-size overflow and a null initial allocation; thread-pool `submit` and
>   `create_pool` reject null promise/pool allocations before placement-new.
> - S5.3f capturing-function allocation: the heap path of
>   `default_function_instance::make2` rejects a null `malloc` before copying
>   the captured environment, matching the `new`/`Box` allocation checks.
> - S5.3e string resize initialization: growing `string::resize` zero-fills the
>   newly exposed bytes (after growth moves the storage) before setting the
>   length, so `get`/`to_view` cannot observe uninitialized characters. A main
>   suite test checks the grown bytes and length.
> - S5.3d self-aliasing string append: `append_with_len` records the source
>   offset when the source points into the destination's storage and recomputes
>   it after growth, so `s.append_view(&s.to_view())` remains valid across
>   reallocation. Tested by doubling a heap-backed string and comparing both
>   halves.
> - S1.10b deque reference soundness: `deque::get_ref` now returns `&T` (it
>   returned `&mut T` from `&self`), and `deque::get_mut` provides mutation with
>   a mutable receiver. A negative test rejects `*d.get_ref(0) = ...`; a
>   positive test covers `get_mut`.
> - S1.8f `Rc`/`Arc::get_mut` weak-handle check: `get_mut` now requires both a
>   single strong owner and zero weak handles, because a `Weak`/`ArcWeak` could
>   otherwise upgrade while the mutable borrow is live. Tests cover a weak
>   handle blocking `get_mut` for both owners.
> - S1.8e `Rc`/`Arc` moved-from guards: after `try_unwrap` sets the owner to
>   null, `clone`/`downgrade`/`get`/`strong_count`/`weak_count` panic instead of
>   dereferencing null; `Weak`/`ArcWeak` count accessors do the same. Runtime
>   tests cover a second `try_unwrap` on both owners.
> - S1.8d `Box` moved-from guards: `into_inner` sets the box to empty, and
>   subsequent `get`/`get_mut`/`into_inner` calls panic instead of dereferencing
>   null. A runtime safety test covers a second `into_inner`.
> - S2.6c `UnsafeCell::get_ref` unsafe: the shared-reference accessor could
>   hand out `&T` while another path mutates the cell, so it is now
>   `unsafe func`; `RefCell::Ref::get` wraps its tracked borrow in `unsafe`, and
>   the Cell test uses `unsafe` explicitly. A negative test proves a safe call
>   is rejected.
> - S2.6b `MaybeUninit`/`ManuallyDrop` unsafe operations: `MaybeUninit::
>   assume_init`/`assume_init_drop` and `ManuallyDrop::take`/`destroy` are now
>   `unsafe func`, so safe code cannot read uninitialized storage or move/drop a
>   manually-managed value without an `unsafe` block. Existing callers already
>   used `unsafe`; negative tests cover the safe-call rejection.
> - S1.9a `Option::take` requires `&mut self`: the old `&self` signature wrote
>   `None` through a shared reference, so a safe API could mutate an immutable
>   binding (and alias a live shared borrow). A negative test proves an immutable
>   receiver is rejected; existing callers already hold mutable locals/fields.
> - S3.0a ownership utilities: `std::mem.replace` returns the old value while
>   moving the replacement in, `std::mem.swap` exchanges two owned values, and
>   `std::mem.forget` suppresses a destructor. They move through raw bytes and
>   explicitly forget the moved-from temporary, so each value is dropped
>   exactly once. The legacy `std::replace`/`std::swap` names delegate to the
>   new implementations. Tests cover destructor-bearing structs, heap strings,
>   the compatibility aliases, and forgetting.
> - S5.3m long constant mutation: `string::set` and `u16string::set` send a
>   long constant view through `ensure_mut` before writing, instead of copying
>   it into the fixed SSO buffer; `u16string::pop_back` and `mutable_data` do
>   the same, `move_const_to_buffer` rejects a length that does not fit, and
>   `append_with_len` checks `offset + len`/`len * 2` before growing.
>   `string::move_const_to_buffer` has the same defensive length check. Tests
>   cover string/u16 set, u16 `mutable_data`, and a u16 constant view longer
>   than the inline buffer.
> - S5.3l string erase overflow: `string::erase` clamps the erase length with
>   `len > size - start` instead of `start + len > size`, so a caller-supplied
>   huge length cannot wrap and move/erase past the string. It also requests
>   `size` (not `size + 1`) from `ensure_mut`, avoiding the same overflow for a
>   maximum-sized string. Tests cover SSO, heap, past-end, and end-of-string
>   erases.
> - S5.3k hash-map node drop: `unordered_map`/`ordered_map` now run the node's
>   generated destructor before freeing it in `delete`, `clear`, and `erase`
>   (instead of explicitly destroying fields through a pointer, which MIR
>   skipped), and `insert` drops the replaced value. Tests verify that
>   replacing a destructor-bearing value drops exactly the old and stored
>   values.
> - S5.3j u16 substring: `u16string::substr` clamps the requested length with
>   `len < size - start` instead of `start + len` (which could overflow for a
>   huge length), builds the SSO/heap result directly, and frees the temporary
>   SSO buffer. A main-suite test asks for `substr(1, SIZE_MAX)` and checks the
>   two remaining units.
> - S5.3i string substring: `string::substring` requires
>   `start <= end <= size()` (so `copy`/substring slices cannot underflow or
>   read past the string), checks `actual_len * 2` overflow, and rejects a null
>   heap allocation. Runtime safety tests cover both invalid ranges.
> - S5.4a static array bounds: a constant index into a fixed-size array is
>   checked at type verification against the declared length and rejected with
>   `SAFE-BOUNDS-001`, on reads and assignment LHS alike.
> - S5.4c dynamic fixed-array bounds: `MIRLowerer` emits a `Compare` +
>   `CondBr` guard (`index < length`, zero-extended to u64 so a negative signed
>   index fails) for every fixed-array index dimension, on reads and stores.
>   Failure calls the extern `abort` (the default panic profile) and then
>   `Unreachable`; the C prelude declares `abort`, LLVM emits the extern
>   declaration, and the interpreter maps it to `host_abort`. Pointer, vector,
>   and struct/operator indexing keep their own checks. Safety-suite runtime
>   tests cover dynamic read and store overruns.
> - S5.5a division/shift checks: integer `/` and `%` reject a zero divisor and
>   signed `MIN / -1` overflow; `<<`/`>>` reject a negative amount or an amount
>   >= the operand bit width. Checks are MIR `Compare` + `CondBr` + `abort` +
>   `Unreachable`, so TCC, LLVM, and the interpreter share them; float division
>   keeps its defined Inf/NaN behavior. The same guards run for compound
>   assignments (`/=`, `%=`, `<<=`, `>>=`) on locals, globals, fields,
>   dereferences, and array elements. Runtime safety tests cover division,
>   remainder, shift, signed overflow, and compound local/field/index cases.
> - S5.5b checked signed arithmetic: `--overflow-checks` traps signed `+`,
>   `-`, `*` and unary negation (including compound assignments) before the
>   wrapped value is stored; unsigned arithmetic keeps its defined wrapping and
>   the flag is off by default so release builds stay two's-complement. The
>   add/sub guards use the width-independent sign-bit identity, mul uses a
>   guarded quotient, and a literal `-2147483648i32` stays valid (only a
>   runtime negation of `MIN` traps). Safety-suite runtime tests cover all
>   four operations, compound forms, and a checked run without overflow.
> - S5.4d string bounds: `string::get`/`set`, `string_view::get`, and
>   `u16_string::get`/`set` reject `index >= size()` with a panic;
>   `string_view::subview` requires `start <= end <= size` and `skip` requires
>   `count <= size`, so a view can no longer point past its source or underflow
>   its length. Runtime safety tests cover string read/write and
>   `string_view` reads, subviews, and skips.
> - S5.4e span bounds: `span::get` rejects `loc >= size()` instead of returning
>   a pointer past the view; a runtime safety test covers the OOB pointer.
> - S5.4f deque bounds/capacity: `deque::get_ptr` rejects `index >= size()`
>   (so `get_ref` cannot dereference null), block growth checks its byte and
>   doubling arithmetic, and a failed block allocation panics instead of
>   writing through null. A runtime safety test covers the OOB index.
> - S5.4b vector runtime bounds: `vector::get`, `get_ptr`, `get_ref`,
>   `get_mut`, `set`, `remove`, `remove_last`, `take_last`, and `last_ptr`
>   reject out-of-range or empty access with `vector index out of bounds` /
>   `vector is empty`. The helpers are `@retained public` because public generic
>   members may only call public functions. `take_last` captures the element
>   pointer before shrinking the length so its own check observes the old size.
>   Runtime safety tests cover an OOB index, empty `pop_back`, and empty
>   `last_ptr`; this also fixes the router-plugin `take_last` path that
>   previously aborted compilation.
> - S1.4h `Option<T>`/`vector<T>` `Clone`: both impls use an inline
>   `<T : Clone>` bound (not a trailing where-clause) so the bound is visible
>   while the generic impl body is linked. `Option` clones through a
>   `&raw mut value as *mut T` pointer, so there is no shallow-copy temporary
>   and no `intrinsics::forget`/leaked copy; `vector` clones each element
>   through a pointer + `memcpy` helper with the same inline bound. Tests cover
>   Some/None, int/string deep copies, empty vectors, and exactly-once drops for
>   a destructor-bearing element type.
> - S1.5e generic-interface method dispatch: `provide_child` now resolves a
>   method on a bare generic `T` through its trait bounds (including generic
>   interfaces such as `Serializer<...>`), returning the interface declaration
>   for symbol resolution. Generic instantiation relinks the call to the
>   concrete impl. Where-clause traits on free generic functions are also
>   appended to the parameter's trait list so `where T: Clone`/`T: Show` method
>   calls resolve; struct/variant member-function where clauses deliberately
>   stay out of the container parameter's traits to avoid leaking per-method
>   bounds. Tests cover generic `Clone` dispatch for primitives and user types
>   and generic `Show` dispatch.
> - S1.5f substitution-aware generic-field lookup: while linking a generic impl
>   body, `ChildResolver::active_param_aliases` maps the target struct's
>   parameters to the impl's parameters (e.g. `Wrapper`'s T -> `impl<T: Show>`'s
>   T). `provide_child` consults the alias when the struct parameter has no
>   traits of its own. This makes `self.value.show()` inside
>   `impl<T: Show> Show for Wrapper<T>` and `self.value.clone()` inside
>   `impl<T: Clone> Clone for Wrapper<T>` resolve and relink to the concrete
>   impl after monomorphization. Tests cover both.
> - S1.5g generic-impl bound demand policy: speculative impl discovery (empty
>   member filter, used to populate a type's method table) no longer reports
>   bound failures; only an explicit member demand does. Impl arguments are
>   checked exactly, without stripping pointers/references, so a `*mut T`
>   argument is not silently treated as `T`. This keeps dormant generic impls
>   from failing compilations that never use the guarded member.
> - S1.5h generic variant/union field dispatch: the target-param alias map now
>   covers `GenericVariantDecl` and `GenericUnionDecl` (not only structs), and
>   `provide_child` falls back to an identifier's type when its linked node has
>   no child (pattern variables, fields). Alias lookup also falls back to the
>   parameter identifier for copied parameter nodes. A local `CloneBox<T>`
>   variant with `impl<T: Clone> Clone for CloneBox<T>` now clones through a
>   pattern variable (`value.clone()`), with a passing test. Generic `Option`/
>   `vector` `Clone` still needs instantiation-time alias propagation for
>   non-Clone element types, so those impls remain out of the tree.
> - S1.5i generic negative impls: `impl<T> !Send for Wrapper<T>` and
>   `impl<T> !Sync for Wrapper<T>` register under the generic target
>   declaration and apply to every instantiation. A concrete
>   `Wrapper<...>` is rejected at a `Send`/`Sync` bound with "is declared
>   !Send/!Sync". Conditional forms with parameter bounds or a where clause
>   (`impl<T: Clone> !Send for Wrapper<T>`,
>   `impl<T> !Sync for Wrapper<T> where T: Clone`) are evaluated against the
>   concrete arguments at the use site and apply only when the bounds hold.
>   Positive/negative tests cover the unconditional, inline-bounded and
>   where-clause forms, including the case where the bound fails and the
>   structural derivation is used instead.
> - Still pending in S1.5: data-race enforcement.
> - S1.6a `std::UnsafeCell<T>` / `std::Cell<T>`: interior mutability is
>   available without weakening the auto traits. `UnsafeCell<T>` is `!Sync`
>   (unconditional negative impl) and `Send` follows `T: Send` structurally;
>   `Cell<T>` wraps it and exposes `get`/`set`/`get_mut`/`as_ptr` with a
>   `T: Copy` read bound. Structural `Send`/`Sync` derivation now consults the
>   implementation index, so an unconditional generic negative impl is honored
>   through nested fields. Positive tests cover interior mutation; negative
>   tests prove `UnsafeCell<int>` is not `Sync` and is `Send`.
> - S3.0a stable move diagnostics: the existing move tracker's errors now carry
>   `SAFE-MOVE-001` (move of an already-moved value, access after move, and
>   assignment after move). Tests cover double move and field access after a
>   move. The tracker still runs at symbol-resolution time; replacing it with
>   MIR move/init dataflow remains M3.1–M3.6.
> - S3.2c callback capture moves: a non-Copy by-value lambda capture is owned by
>   the reusable callback environment. Returning it, initializing a local from
>   it, assigning it, or passing it by value is rejected with `SAFE-MOVE-002`;
>   the old lowering bitwise-copied the capture, which could double-own its
>   storage on repeated calls. Full once/reusable callback semantics still need
>   the `FnOnce`/`FnMut`/`Fn` traits.
> - S3.2b array index moves: moving a destructor-bearing element out of an
>   array by index is rejected with `SAFE-MOVE-001`, for both constant and
>   dynamic indices. This closes the copy-out/double-drop hole (`var x =
>   arr[i]` used to bitwise-copy the element while the array still ran its
>   element destructors). Whole-array moves, field reads (`arr[i].field`), and
>   `Copy` element indexing remain allowed. Field-wise drop elaboration for
>   partial array moves is still pending.
> - S3.1a move-path trees: every owned place (a destructor-bearing aggregate or
>   an array of one) now gets a `MIRMovePathNode` tree for its owned fields and
>   fixed-array elements. Link fields use `MIR_INVALID_ID` for "none" (path 0 is
>   valid), and the builder keeps root/child states in sync on
>   init/copy-init/move-init/assign/drop/destroy. The MIR verifier rejects
>   out-of-range links, self-parenting, projection indices on a root, a root not
>   referenced by its place, and non-terminating sibling lists. `MIRTests`
>   covers a struct + array-of-destructor tree and move-state propagation.
> - S3.1b/S3.2 MIR init/move dataflow: `MIRMoveCheck` propagates
>   Uninitialized/Initialized/Moved/Destroyed over each function's CFG and
>   reports `SAFE-INIT-001`/`SAFE-MOVE-001` for moves/drops/destroys/reads of
>   dead or conditionally initialized paths. Disagreeing joins become `Maybe`
>   and are rejected by reads, moves and unconditional drops/destroys; a
>   conditional `Drop` is accepted and its flag is resolved when the lowering
>   used a constant `set_drop` (a false flag is a no-op, a true flag requires
>   an initialized path). Parameters/globals start initialized; coroutine frame
>   fields are left to the async lowering. The pass runs for every lowered
>   function in both normal and `--check` builds before either backend sees the
>   MIR, so `unsafe(...)` cannot disable it. `MIRTests` covers valid moves,
>   double moves, drop of an uninitialized path, and conditional drop; the
>   negative suite covers the `unsafe` bypass and conditional-init use.
>   Element-assignment lowering now marks the moved source before
>   `element_store_place`, and `archive::zip_contains`/`tar_contains` initialize
>   their out-parameter entry before passing it to the finder.
> - S3.1c field-wise initialization state: the "writing one member initializes
>   the whole struct" heuristic is gone. A projection store resolves the exact
>   move-path node: a write into a moved/destroyed value is `SAFE-MOVE-001`,
>   and reading a chained field (`s.b.id`) is checked through the derived
>   `field_addr` path.
> - S3.1d field-wise drop elaboration: a bare local of a struct with generated
>   drop glue (`var s : S`, no initializer) gives every owned field its own
>   drop flag, including one level of generated-glue grandchild
>   (`s.inner.x`). A fixed array of destructible elements (<= 64) likewise
>   gets one flag per element. A write runs the old value's destructor only
>   when the target flag is live, then arms it; scope exit, `break`/`continue`
>   and reassignment destroy only the initialized entries (array elements
>   last-to-first, struct fields in declaration order). A whole-value
>   assignment, whole-field move or `forget` arms/clears the whole subtree.
>   A struct with a user `@delete`, a large array, a dynamic array index and a
>   projection deeper than one extra level still require the destination to be
>   initialized. A mutable raw pointer escaping from the value (`&raw mut x`,
>   `&raw mut x[i]`) switches it to the whole-value drop and makes the
>   dataflow trust the place (the `std` `Option::take` / `archive::extract_entry`
>   pattern); a 2-operand place-to-place store still requires a live source
>   (rejecting a partially initialized return). `MIRMoveCheck` only accepts a
>   partial write when the destination place carries
>   `MIRPlaceDef::field_wise_drop`. The lowering also marks a pointer-based
>   element store's source moved (fixing a double drop in `vector::set`).
>   `unsafe` bulk memory (`memcpy`/`memmove`/`memset`, direct or via an
>   aggregate sret return) initializes its destination. Partial moves demote
>   ancestors to `PartiallyMoved`, so a whole-value drop/move of a partially
>   moved aggregate is rejected. Unions are raw storage
>   (`type_needs_move_path` does not recurse into union fields) and are tracked
>   only when the union itself declares a destructor. `MIRTests` covers
>   construction temporaries, direct field init, user-destructor rejection,
>   field writes after a move, union paths, the nested-field read and array
>   partial init; the safety suite checks the runtime drop set/order
>   (`partial_init_destroys_only_initialized_fields`,
>   `full_field_init_destroys_every_field`,
>   `partial_init_reassignment_destroys_old_value`,
>   `partial_array_init_destroys_only_initialized_elements`,
>   `partial_array_reassignment_destroys_old_element`).
> - S3.3 variant ownership contract: a variant is one owned value. Its
>   generated destructor reads the tag and destroys only the active payload, so
>   MIR gives the variant a root move path with no payload children; moving a
>   payload out stays a front-end pattern-match effect. Unions remain raw
>   storage. `MIRTests` covers both roots.
> - S3.4/S4.6b dropck: the declaration-shape `SAFE-DROP-002` warning is retired.
>   Per-use dropck binds a destructor-bearing aggregate returned from one
>   reference argument to that loan; `SAFE-LIFE-002` checks the guard's drop
>   against the referent's storage end, including variant payloads
>   (`Option<Ref>`/`RefCell`).
> - S3.6 single init/move source: the flow-sensitive AST definite-assignment
>   pass and the AST move tracker are removed. `MIRMoveCheck` is the sole
>   producer of init/move/drop state. A `MoveOut` MIR marker checks by-value
>   call arguments (uninitialized/moved), the lowering clears a moved pattern
>   payload's source variant, and a `TypeVerify` check rejects moving a
>   destructor-bearing array element by index (`SAFE-MOVE-001`). The
>   module-scope uninitialized-global check remains in `TypeVerify`. First
>   assignment now destroys the old value through the runtime drop flag instead
>   of `is_first_init`. `MIRTests` is `4585 checks`. `SAFE-DROP-002` is retired
>   (per-use dropck uses `SAFE-LIFE-002`; the code is not reused).
> - S1.5m conditional positive auto-trait impls: `unsafe impl<T> Send for
>   Box<T> where T: Send` is evaluated against the concrete arguments at a
>   `where T: Send` use site (and recursively for nested boxes), instead of
>   falling back to the raw-pointer field graph. Negative impls still take
>   precedence.
> - S1.8 `std::Box<T>`: a unique heap owner over `malloc`/`delete` with
>   `get`/`get_mut`/`as_ptr`/`as_mut_ptr`, byte-moving construction
>   (`memcpy` + `intrinsics::forget`), and `Send`/`Sync` following `T` via
>   conditional unsafe impls. Tests cover drop-once, mutation, moves, strings,
>   nested Send, and non-Send/non-Sync inner values.
> - S1.8b `std::Rc<T>`: single-threaded reference counting with a private
>   `RcInner<T>` (strong count + value), `clone`, `get`, `strong_count`,
>   `get_mut` and `try_unwrap` (both require a unique owner), `Weak<T>`
>   (`downgrade`/`upgrade`, strong/weak counts), and exactly-once destruction
>   of the shared value. The value lives in `ManuallyDrop` so weak handles can
>   keep the allocation alive after the last strong owner; `Rc`/`Weak` are
>   structurally `!Send`/`!Sync`. `ManuallyDrop::destroy` and
>   `MaybeUninit::assume_init_drop` destroy in place.
>   `Box::into_inner` moves the value out and frees its storage without running
>   the destructor twice.
> - S2.6a unsafe functions: `unsafe func` / `public unsafe func` parse at
>   top level and as struct/variant members (including `unsafe async func`).
>   `unsafe(expr)` and `unsafe { }` open an unsafe context; calling an unsafe
>   function from a safe context is a type error. Method calls are covered
>   through the linked function declaration. Tests cover expression/block
>   calls, unsafe methods, and both direct-call negative cases.
> - S2.6d unsafe invariant constructors: `vector::set_len` is now
>   `unsafe func` (growing past capacity exposes uninitialized elements and
>   shrinking skips the tail destructors), and the audited
>   `fs::read_file` caller wraps it in `unsafe`. `default_function_instance
>   .make2` (which turns an arbitrary code pointer, capture pointer and
>   destructor into a `std::function`) is `unsafe func` too; its only caller is
>   the audited `make` lambda path, which wraps the call in `unsafe`.
>   Safe-context calls to an `unsafe func` now carry the stable
>   `SAFE-UNSAFE-002` code, and the negative tests assert that code for
>   expression calls, method calls, and both new constructors.
> - S1.8c `std::Arc<T>` / `ArcWeak<T>`: thread-safe reference counting with
>   `clone`, `get`, `get_mut`, `try_unwrap`, `downgrade`, `upgrade`, and
>   strong/weak counts. Counts are
>   currently protected by OS mutexes (`ArcCount`) rather than atomics, so the
>   safe API and ownership rules match `Arc` without pulling the atomic library
>   into `std`; the count implementation can be swapped later. `Arc<T>` is
>   `Send`/`Sync` when `T: Send + Sync`; `ArcWeak<T>` is the flat-namespace
>   counterpart of `Rc`'s `Weak<T>`. Tests cover sharing, weak upgrade/expiry,
>   and auto-trait bounds.
> - S1.6e `std::Mutex<T>` / `MutexGuard<T>`: OS-mutex-backed exclusive
>   access with `lock`/`try_lock`, `get_mut`, and a guard that unlocks from its
>   destructor. `Mutex<T>` is `Send`/`Sync` when `T: Send` through conditional
>   unsafe impls; guards stay thread-bound (raw pointers). Tests cover
>   exclusion, try-lock failure, release on scope exit, and auto-trait bounds.
> - S1.6f `std::RwLock<T>` / read-write guards: multiple concurrent readers or
>   one exclusive writer, `read`/`try_read`/`write`/`try_write`, `get_mut`, and
>   guards that release from their destructor. Storage is a native
>   `pthread_rwlock_t` on POSIX and an `SRWLOCK` on Windows. `RwLock<T>` is
>   `Send` when `T: Send` and `Sync` when `T: Send + Sync`; tests cover shared
>   readers, writer exclusion, release on scope exit, mutation visibility, and
>   the auto-trait bounds.
> - S1.6d `std::RefCell<T>`: runtime borrow tracking with `try_borrow` /
>   `try_borrow_mut` returning `Option<Ref<T>>` / `Option<RefMut<T>>` instead
>   of panicking. Guards release the state from their destructors, shared
>   borrows may coexist, and a mutable borrow is exclusive. The state is a
>   plain `int` mutated through a raw field pointer, so it has no generic
>   pointer-type inference issue; `RefCell<T>` is `!Sync` and `Send` follows
>   `T` through `UnsafeCell`. Tests cover shared overlap, mutable exclusion,
>   release on scope exit, and auto-trait bounds.
> - S1.6c `std::MaybeUninit<T>` / `std::ManuallyDrop<T>`: raw storage with
>   no automatic drop glue. `@no_drop` suppresses both generated and explicit
>   destructors (leaking is safe); `MaybeUninit::write`/`assume_init` and
>   `ManuallyDrop::take` move bytes with `memcpy` + `intrinsics::forget`.
>   `@no_drop` is copied to generic instantiations. Tests cover write/overwrite,
>   unwritten storage, drop suppression, ownership transfer, and `Send`
>   following the inner value.
> - S1.6b `std::NonNull<T>`: a non-null raw-pointer wrapper with a private
>   field, safe `non_null`/`non_null_try`, explicit unsafe
>   `non_null_unchecked`, `as_ptr` and `is_null`. It is neither `Send` nor
>   `Sync` (the field is a raw pointer), matching the conservative auto-trait
>   rule; tests cover construction, null rejection, pointer identity and writes
>   through `as_ptr`. Private-field initialization is rejected cross-module
>   (`SAFE-VIS-001`), and unsafe functions may establish the invariant only at
>   an explicit boundary.
> - S1.5l nested auto-trait overrides: structural `Send`/`Sync` derivation now
>   honors an explicit positive impl on a nested field's container (for example
>   `unsafe impl Send for Raw` makes a struct containing `Raw` Send even though
>   `Raw` stores a raw pointer). Negative impls are checked first, so a
>   conflicting override cannot become a false positive.
> - S1.5k nested generic-impl body deferral: a generic impl instantiation whose
>   arguments still contain a generic parameter is registered and
>   signature-finalized, but its body is not linked until the parameter becomes
>   concrete. This removes the nondeterministic "unresolved child 'clone'"
>   failures seen when a nested `Option<T>`/`vector<T>` body was linked too
>   early. The container `Clone` impls now use inline `<T : Clone>` bounds plus
>   cast/pointer helpers (S1.4h), so their bodies resolve deterministically.
> - S1.10a vector reference soundness: `vector::get_ref` now returns `&T`
>   (it used to return `&mut T` from `&self`), and `vector::get_mut` provides
>   mutation through a mutable vector. A negative test proves
>   `*v.get_ref(0) = ...` is rejected; a positive test covers `get_mut`.
> - Still pending in S1.4: automatic `Clone` derivation/body generation.
>
> M3 is complete. M4 started (S4.2a): `MIRRegionSignature` (per-parameter
> region, return region, `'a: 'b` outlives, HRTB binders reserved) is built
> from declarations before body lowering and stored in the module-lifetime
> `MIRModule::region_signatures` side table; `MIRFunction::region_signature`
> links to it, `MIRDump` prints it for golden tests, and `make_region_signature`
> maps `'static`/elided/`'self` names (quoted or bare). `MIRTests` covers the
> mapping/table/dump. S4.3a borrow lowering: `Borrow`/`BorrowMut` MIR opcodes
> (address value + region placeholder, reference-typed result) are emitted for
> `&x`/`&mut x`/`&*p`/`&field`/`&arr[i]`; raw `&raw` stays `AddressOf`. C/LLVM
> pass the address through, the interpreter copies it. `MIRTests` is
> `4610 checks`. S4.4a report-only loan analysis: `MIRBorrowCheck` tracks
> `Borrow`/`BorrowMut` loans through reference locals and reports overlapping
> live loans (`SAFE-BORROW-001`) and moves/drops of a borrowed place
> (`SAFE-BORROW-002`) as warnings. Loans passed to a call are marked escaped
> and skipped until the interprocedural lifetime check (S4.5). S4.4b: loan
> liveness is now a CFG worklist (`live_in`/`live_out` per block) with a
> backward per-block walk for conflict/move positions, so disjoint branches
> do not conflict; loading a reference local is a use. Each borrow gets a fresh
> function-local region id (`MIRFunction::region_var_count`, seeded from the
> signature's declared lifetimes). `MIRTests` is `4617 checks`; negative tests
> cover mutable/shared alias, sequential, shared, move-while-borrowed and
> disjoint-branch cases. S4.5a callee signatures: the per-symbol region table is
> filled at interning time, so a known callee that returns a reference extends
> the single reference argument's loan through the result; a
> non-reference-returning callee ends it at the call. Indirect/extern callees
> and multiple reference arguments stay conservative (escape). Four report-only
> `SAFE-BORROW-001` hits remain in generated test-harness code (`__chemda_*`)
> for the S4.9 migration. S4.5b reference escape on return: `SAFE-LIFE-001`
> reports returning a reference to a local/temporary/coroutine frame field; a
> returned parameter/global reference is allowed. It found a genuine dangling
> reference in the test suite (`get_destruct_or_not` returned `&d` to a local);
> the dead function was removed and **SAFE-LIFE-001 is enforced**. `MIRTests`
> is `4619 checks`; negative tests cover local/parameter/ref-return calls (450
> tests). The server/tls suite also exposed two M3 refinements: a user-`@delete`
> local may be built field by field when it is moved/returned as a whole
> (partial values leak, safe; partial moves stay rejected), and
> `*ptr = owned_local` now marks the source moved (fixing a double drop).
> S4.4c storage lifetimes: the lowering emits `StorageLive`/`StorageDead` for
> every local in each lexical scope (no runtime effect). The loan analysis
> reports `SAFE-LIFE-002` when a loan is still live at the referent local's
> `StorageDead`; references stored in struct fields are followed through field
> stores and whole moves. `MIRTests` is `4620 checks`; negative tests cover the
> struct-field escape and the same-scope positive (452 tests). SAFE-LIFE-002 is
> **enforced**. The universal suite exposed 40 loop back-edge false positives;
> `block_use` now uses a gen/kill computation (uses before the block's
> definition), and universal is 368/368 with zero warnings. S4.6a return-region
> outlives: `SAFE-LIFE-003` (enforced) requires a returned parameter reference
> to outlive the declared return region (identity, `'static`, or a transitive
> `where 'a: 'b` edge). Parameter places carry `param_index`; reference values
> and places propagate it through Load/Store/Copy/Cast/MoveInit. The borrow
> pass always runs (no "has Borrow" fast path) so a pure `return x` is checked.
> `MIRTests` is `4630 checks`; negative tests cover the missing/satisfied
> bound (454 tests). Aggregate returns are covered too: field stores record
> `field_params`/`field_loans`, whole-value moves transfer them, the sret return
> recovers the stored source place, and generic return types contribute their
> first lifetime argument (`Holder<'a>`) to the signature. Negative tests cover
> aggregate-return region, local-field escape, the satisfied bound, and a direct
> field return. S4.5 multi-reference mapping: a reference-returning
> callee with several reference arguments maps the result to the argument whose
> region is the return region (identity, then a unique outlives source); the
> migrated two-`&mut` test leaves the in-tree tree with zero BORROW hits.
> Ambiguous multi-reference results carry an alias set of all candidate loans,
> so any use keeps every candidate live and a return checks all their regions.
> Lambda diagnostics carry their enclosing function and source location in
> `debug_quick`; `MIRTests` is `4632 checks`, negative tests `466`. After the
> migration reached zero hits in every suite, `SAFE-BORROW-001`/`002` were
> enforced as hard errors (rendered as `error[SAFE-*]`). Autoref is tracked:
> reference receivers lower to `Borrow`/`BorrowMut`, and method arguments are
> evaluated before the receiver borrow is activated (two-phase approximation),
> so `v.push(v.len())` works while a live `&v` + `v.push(2)` is rejected.
> Known-callee detection uses `MIRSymbolRecord::has_body`, and pre-declared
> symbols carry their type and region signature. Migrated
> `universal_cbi/router/emit.ch` (read before `get_mut`). Per-use dropck:
> destructor-bearing aggregate results from one reference argument carry that
> loan, so `SAFE-LIFE-002` rejects a `Ref`/`RefMut` (even through `Option`)
> that outlives its cell; `SAFE-DROP-002` is retired. Provenance reporting:
> `SAFE-UNSAFE-001` only flags raw-pointer dereferences, and `SAFE-PROV-001`
> flags `&*p`/`&mut *p` and raw/int/null → reference casts; both are now
> enforced after migrating the parser/TLS/atomic/test helpers (function-level
> unsafe boundaries; expression-level inside async functions).
> S4.1 (partial): `core::Unpin` is a structural safe auto trait, `PhantomPinned`
> is `!Unpin` via a negative impl, and `core::Pin<T>` has a private pointer,
> safe `new`/`pin` (`T: Unpin`), unsafe `from_raw`, `get_ref`, `get_mut`, unsafe
> `get_unchecked_mut`, and `into_inner`; `Waker` is explicitly
> `unsafe impl Send + Sync` (its vtable
> contract requires thread-safe wake/clone/drop). Waker data/vtable fields are
> private, safe code uses `Waker.empty()` (null-safe wake/clone) or
> `unsafe Waker.from_raw(...)`, and `is_empty()` replaces raw vtable checks.
> `SAFE-VIS-001` rejects struct-literal initialization of a private member
> outside its type, so the fields cannot be bypassed; async frame fields remain.
> `Chunk<T>` is similarly unconstructible in safe code; `chunk_unchecked` is
> the unsafe boundary used by trusted iterators.
> M5.1 (partial) validity: `SAFE-VALID-001` rejects `zeroed<T>()` when the
> concrete type contains a reference field (direct, array, variant payload, or
> a specialized generic field such as `W<&int>`); `PhantomData<&T>` is
> storage-free and exempt. `zeroed<T>()` now requires `T: Zeroable`; the
> explicit `zeroed:unsafe<T>()` unchecked form remains temporarily for audited
> FFI/TLS storage until those call sites are migrated to `unsafe impl Zeroable`.
> S2.6 FFI enforced: `SAFE-FFI-001` rejects `@extern` calls in safe context.
> The whole in-tree tree is migrated to per-call `unsafe(...)` expressions or
> `unsafe { ... }` statement blocks (883 library calls plus tests/build.lab);
> the negative test expects the error and the unsafe form is covered. The
> audit baseline grew by the new explicit unsafe boundaries (263 blocks/299
> values in the safety module), reviewed and updated intentionally.
> S4.2b aggregate field regions + variance: `compiler/safety/Variance.h`
> computes reference/pointer/array/function/generic variance (`&mut T` and
> `*mut T` pointees invariant, `Cell`/`UnsafeCell` invariant, unions invariant)
> and the lifetime-parameter occurrence set of a field type.
> `MIRRegionSignature::param_field_regions` records, per aggregate parameter,
> each direct reference field's interned name constant and the declared region
> resolved through the parameter type's lifetime arguments. The borrow checker
> seeds caller-owned field loans, follows `p.field` and a method's
> `self.field` (through a loaded reference parameter), and `SAFE-LIFE-003` now
> rejects returning a field whose lifetime argument does not outlive the
> declared return region. Positive/negative tests cover by-value aggregate
> parameters, `&self` methods, and the satisfying `where 'a: 'b` edge.
> `region_signatures` is a mutex-copied deque, so parallel lowering cannot
> observe a dangling reference while another worker interns a signature.
> S4.5 precision: borrow overlap uses constant field/array projections, so
> `a[0]`/`a[1]` are disjoint even for non-owning element types while a dynamic
> index still overlaps; reborrowing through `&mut *r` is accepted.
> `MIRTests` is `4638 checks`; negative tests `491`.
> S4.2b view/async soundness: borrowing views (`string_view`, `span`,
> `Chunk`) now carry their source loan across call and `Init` boundaries, so a
> view of a local (or of `&raw` array storage) cannot escape
> (`SAFE-LIFE-001`), while a view of a parameter, a `'static`-returning
> allocator (`BatchAllocator.allocate_view`) or a same-scope local is allowed.
> `FutureHandle` fields are private; construction is the unsafe
> `future_handle_raw`/`set_raw` boundary, so safe code cannot forge the
> frame/vtable pair (`SAFE-VIS-001`). Async poll/resume functions treat
> reference frame fields as live loans and reject moving/dropping another
> frame field while the reference is still used (`SAFE-BORROW-002`); a borrow
> that ends before the move is accepted. A view built from a pointer-returning
> method (`s.data()`) inherits the receiver's loan too. `MIRTests` is `4638
> checks`; negative tests `498`; async suite `54`.
> S4.2/S1.2 temporary lifetime extension: a call result borrowed by a `var`
> initializer (`var r = &make()`, `var h = H { r : &make() }`) is now a
> block-local destructible rather than a statement temporary. The extended
> temporary is inserted before its owner in the cleanup order, so the owner
> drops first and the temporary stays alive until the owner is gone.
> Extension does **not** apply through call arguments or later assignments:
> those temporaries are destroyed at the end of the statement, and statement
> temporaries now emit `StorageDead`, so `SAFE-LIFE-002` rejects a reference
> that outlives one (`f(&make())` used later, or an aggregate assigned a
> reference to `&make()`). Reading a reference field through `FieldAddr` +
> `Load` now counts as a use of the field's loan, so the escape is detected
> even when the field is reached through an address. Runtime tests cover the
> destructor order for direct and aggregate borrows; negative tests cover both
> non-extending forms. Safety suite `55`; negative tests `613`.
> S4.2 higher-ranked function types: `for<'a, 'b> (x : &'a T) => R` parses into
> `FunctionType::binder_lifetimes`; the binders are in scope only inside that
> function type, duplicates are rejected, and `SAFE-BORROW-099` rejects an
> unknown/nested use. Function-type satisfaction treats an elided or generic
> source lifetime as instantiating the target binder, but rejects a concrete
> `'static`/outer lifetime (it is more restrictive than `for<'a>`). A
> lifetime-only generic function used as a value (`func <'a> read(x : &'a T)`)
> is instantiated once and can be passed to a higher-ranked callback.
> `MIRRegionSignature::hrtb_binders` records a declaration's own lifetime
> parameters, and `MIRDump` prints them.
> **HRTB call regions:** a `CallIndirect` records the source function type's
> region signature per instruction (`MIRFunction::call_region_signatures`).
> The borrow checker binds a borrowed result to the argument whose declared
> region matches, so `for<'a> (&'a T) => &'a T`, one mutable input, and
> multi-borrow local callbacks work. Multi-borrow callbacks pin every local
> referent loan until function exit; a parameter/global referent is rejected
> with `SAFE-BORROW-099` because a retained borrow could outlive it in the
> caller. Nested-aggregate HRTB remains explicitly rejected.
> **Static lifetime enforcement:** assigning a shorter reference or generic
> lifetime argument to a `'static` target is `SAFE-LIFE-004`. This closes the
> container/dropck hole where a `vector<G<'static>>` could accept a `G<'a>`
> built from a stack borrow. Safety suite `55`; negative tests `613`;
> `MIRTests` `4643 checks`.
> M5.5b float→int: every `as` cast from float/double to an integer width now
> lowers to explicit blocks that map NaN to zero and saturate values below the
> destination minimum / above its maximum, instead of C/LLVM out-of-range
> `fptosi` UB. LLVM float `Ne` is `fcmp une` (C semantics: `NaN != NaN`), so
> the NaN branch is also correct there. Runtime tests cover large positive and
> negative values, unsigned saturation, in-range truncation, and NaN.
> S5.6a panic runtime ABI: every module, including a bare module, carries
> `__chc_panic`, `__chc_bounds_fail`, `__chc_overflow_fail`,
> `__chc_div_zero_fail`, `__chc_shift_fail` and `__chc_alloc_fail`. MIR checks
> call the reason-specific entry with the current source file/line/column, and
> the default abort profile prints `panic: <reason> at <file>:<line>:<col>` on
> both C and LLVM before aborting. `std::panic::raw_panic` routes through
> `__chc_panic`; the profile never unwinds, so FFI boundaries cannot be crossed
> by an unwind. Runtime tests assert the bounds reason and panic message.
> The audit baseline records the new explicit `__chc_panic` boundary and the
> thread/async unsafe wrappers (audit totals: `extern_decls` +1,
> `raw_pointer_types` +2, `unsafe_values` +9; the removed `printf`/`abort`
> body drops one variadic call and the last `abort_calls` count).
> S6.1a raw thread/async submission: `std::concurrent.spawn`,
> `ThreadPool.submit`/`submit_void`, and `async::spawn`/`spawn_local`/
> `spawn_blocking` are now `unsafe` boundaries. Their captures are erased
> (`*void`, `std::function`, coroutine frames), so the compiler cannot prove
> `Send`/`'static`; safe submission would move a non-`Send` capture to another
> thread. `Thread`'s handle is private and construction goes through the unsafe
> `Thread.from_raw`, so safe code cannot forge a thread handle either. All
> in-tree call sites were migrated to explicit `unsafe(...)`. This slice also
> fixed `SAFE-UNSAFE-002` for generic functions: the parser marked only a
> concrete `FunctionDeclaration`, so `unsafe func <T> ...` (and
> `future_handle_raw`, `spawn`, ...) were callable from safe code through the
> `GenericFuncDecl` wrapper. `unsafe(...)` is now also accepted as an
> expression statement. Negative tests cover generic unsafe calls, raw thread
> spawn, async spawn, `Thread.from_raw`, and forged thread handles (`503`).
> The implicit `span`/`string_view` constructor selection could choose an
> overload for an unresolved value under some module orderings. The
> `InterpretScope` identity cache, bounded reference-chain resolution,
> array-return comptime folding and const-initializer preference now make it
> deterministic; repeated TCC/LLVM lib runs reached `719/719`. See the M5.2a
> status entry below.
> S5.3/S2.5 safety guard: `Layout` now keeps its size/alignment private,
> exposes `size()`/`align()`/`validate()`, and `GlobalAllocator` rejects a
> zero/non-power-of-two alignment or a size above `isize::MAX` before touching
> memory. `SAFE-FFI-002` still rejects an `@extern` function that passes or
> returns a Chemical **variant** by value (no C layout). Plain structs/unions
> now use the x86-64 SysV C ABI on both backends: small values are coerced into
> integer/SSE registers, larger values use byval/sret memory, and mixed
> struct-plus-scalar signatures are supported. Other target ABIs still need
> classification; pointer-based FFI remains allowed. `std::default_function_instance` also keeps its erased
> code/data/destructor fields private: safe construction goes through `make`,
> raw `make2` stays `unsafe func`, and a struct literal outside the type is
> `SAFE-VIS-001`. A runtime test covers the invalid-layout panic and negative
> tests cover the aggregate FFI rejection and callback forgery. Safety suite
> `53`; negative tests `535`; `MIRTests` `4638 checks` (historical slice).
> M5.2a validity guards: `SAFE-VALID-002` rejects an integer/float/pointer to
> `bool` cast in safe code (the `atomic` CAS helpers now use `!= 0`);
> `SAFE-VALID-003` rejects an integer literal whose value is not a declared
> enum member; `SAFE-VALID-004` rejects `zeroed<Enum>()` when no member has
> value 0. M5.2b is implemented: a runtime integer-to-enum cast is checked in
> MIR lowering against every declared member (including inherited members) and
> an invalid tag aborts through `__chc_enum_fail` with the source location.
> `SAFE-VALID-005` is retired; the literal-only `SAFE-VALID-003` check remains.
> `SAFE-VALID-006` requires an unsafe boundary for `@volatile` reads/writes,
> because volatile is not atomic and concurrent use is a data race.
> `SAFE-VALID-007` requires an unsafe boundary for named union field access and
> public anonymous-union access, because the active union member is not tracked.
> `SAFE-VALID-008` rejects creating a reference to a field of a packed aggregate
> (the field's ABI alignment is 1, so the reference would not satisfy the
> referent's alignment); value reads/writes through a packed field remain
> allowed and use byte-aligned loads/stores.
> The raw-pointer `core::stream::Stream::writeStr`/`writeStrNoLen` methods are
> `unsafe func`; `std::print`/`println`'s generated `CommandLineStream` writes
> are explicitly wrapped in an `UnsafeBlock`. The command-line buffer
> conversion helpers are internal, so safe code cannot call them with an
> arbitrary output pointer.
> The in-tree lexer/parser/plugin casts (`js_cbi_lexer`, `universal_parser`,
> `css_parser`, `json_cbi`, the test IPC parser) were replaced with integer
> comparisons or total mapping helpers instead of forging an enum. `NonNull`, `Box`, `Rc`/`Weak`, and `Arc`/`ArcWeak` now have private
> pointer fields and checked/unsafe constructors; `vector`, `deque`, `span`,
> `string_view`, map iterators and map storage, raw OS lock storage, and
> `Pin`/`Chunk` storage are private/internal, while `string`/`u16string` tagged
> storage and `RefCell`/`Cell`/lock state are private/internal. Direct field
> access and aggregate init from a user module are `SAFE-VIS-001`, closing
> safe-code retargeting/double-free, length/capacity and borrow-state
> corruption paths. General `unsafe interface` support now enforces
> `SAFE-TRAIT-001`: implementing an unsafe interface requires `unsafe impl`.
> `core::iterable::Linear` is an unsafe interface because `data()` feeds for-in
> pointer arithmetic; `Allocator` is unsafe because an impl supplies raw
> allocation pointers. The std vector/span impls are marked accordingly. Nested
> `unsafe impl` is parsed inside container bodies. Raw pointer/length
> constructors for `string`, `string_view`, `span`, and `u16string` are now
> `unsafe func`; a safe caller must use `unsafe(...)` or a checked/literal
> constructor. The in-tree call sites that construct a view of already-owned,
> valid storage were audited and wrapped, so the safety audit baseline was
> updated rather than silently ignoring the growth.
> Compiler comptime determinism: `InterpretScope` now carries a unique id and
> identifier lookup checks the current scope before its cached-depth fast path;
> `resolve_ref` follows a bounded reference chain; `MIRComptime` folds an
> array-returning single-return comptime call through the AST instead of the
> scalar engine bridge; and a comptime-template argument that is a plain local
> `const` uses its AST initializer rather than a shared name lookup. This closes
> the intermittent `span`/`string_view` `intrinsics::size` rejection; TCC and
> LLVM libs were each run repeatedly with `719/719`.
> M5.7a interpreter memory guard: the MIR interpreter keeps a live/tombstone
> range for every host `malloc`/`realloc` block, checks reads/writes (scalar,
> aggregate, `memcpy`/`memmove`/`memset`) before touching memory, and reports
> an interpreter error instead of aborting. Frame places are registered
> individually, and globals/constant blocks/string tables are registered as
> they are created. Each frame arena is tied to its owning interpreter's range
> table (not a thread-local "current engine"), which fixed a real nested-engine
> bug where an outer frame allocation was registered in an inner comptime
> engine and then reported as an out-of-bounds write under some file orders.
> Negative tests cover a comptime heap out-of-bounds write and a use-after-free
> read. M5.7b closes the remaining interpreter boundary: every dereference must
> fall inside a live registered allocation, so unregistered/foreign pointers
> are rejected instead of falling back to host pointer sanity. Frame ranges are
> retained as dead tombstones when a frame is reset/destroyed, which rejects a
> pointer into a returned frame (including the new cross-frame test).
> `strlen`/`strcmp`/`printf` use bounded scans only: there is no host
> `std::strlen` fallback, an unterminated registered block is an error, and
> `%s` is appended with its checked length. Comptime-marshalled string/
> aggregate argument buffers, `call_sret` output buffers, and `argv` are
> registered as foreign ranges for exactly the call/run lifetime. Negative
> tests cover a foreign integer pointer, a returned-frame pointer, and a foreign
> `printf("%s", ...)` argument.
> S4.8 raw-pointer operations: `SAFE-UNSAFE-001` now covers implicit raw-pointer
> dereferences as well as explicit `*p`. Field/method projection (`p.x`,
> `p.method()`), indexing (`p[i]`, including `vector.data()[i]`), placement new
> (`new (p) T`), pointer arithmetic (`p + n`, `p += n`, `p++`), and raw
> `delete`/`dealloc` all require an unsafe boundary. The check is implemented in `SymResLinkBody` for the
> access chain, index operator, expression, increment, assignment, delete and
> dealloc visitors. Generated module-build scripts (`ModToLabConverter`) are
> emitted with one explicit unsafe boundary around the raw build-context/job
> operations. Wrapping generic bodies exposed a missing `ValueNode::copy`, which
> is now implemented so generic expression-statement bodies survive
> finalization. In-tree stdlib/CBI/plugin/build.lab call sites were migrated;
> safety audit baseline is `unsafe_blocks 574`, `unsafe_values 441`.
> S2.2/S4.8 function-pointer safety: function types now carry
> `requires_unsafe`. It is set for `unsafe func` and `@extern` declarations and
> is part of function-type identity, so an unsafe/extern callee cannot be
> silently coerced to a safe function pointer. `SAFE-UNSAFE-002` rejects both a
> call through such a function pointer in safe context and the unsafe-to-safe
> coercion. The type syntax `unsafe (args) => ret` names an unsafe function
> pointer explicitly; a safe callable still coerces to it. Negative tests cover
> variable calls, extern-pointer calls, coercion, and the safe/unsafe type
> boundary. Inline assembly is now enforced as `SAFE-UNSAFE-003`; the atomic
> fence helpers are the only in-tree asm users and wrap each instruction in an
> explicit unsafe block. The C backend no longer relies on C signed-overflow
> UB: signed add/sub/mul/left-shift are computed in an unsigned type and cast
> back to the signed result, so the default/release profile is defined
> two's-complement wrapping; LLVM already emits wrapping integer ops. The
> explicit `--overflow-checks` debug profile still traps add/sub/mul/neg.
> Global/static model: `SAFE-GLOBAL-001` requires an unsafe boundary for every
> access to a mutable top-level `var`, `static mut`, or `@extern` global;
> thread-local and comptime globals are exempt. `SAFE-GLOBAL-002` requires an
> immutable global's type to be `Sync` (immutable globals are shared across
> threads). Contextual `static` / `static mut` syntax is parsed without adding a
> lexer keyword (no CBI token shift): `static` is immutable storage and
> `static mut` is mutable storage. In-tree globals were migrated with audited
> unsafe boundaries or existing synchronization; negative tests cover mutable,
> extern, non-Sync, static, static-mut, and safe/unsafe cases. `atomic_flag._value` is now private and construction goes
> through `atomic_flag.make(value)` (S5.3/type-invariant cleanup). Named unsafe
> flags such as `unsafe "no_lifetime_check"` are removed; `unsafe` only marks a
> boundary and never disables lifetime checking. The temporary-lifetime check
> now always runs.
> S5.1/Zeroable: `core::Zeroable` is an `unsafe interface`. Safe
> `zeroed<T>()`/`= zeroed` now require `T: Zeroable` (`SAFE-ZERO-001`).
> Integers, floats, bool, raw pointers, arrays, all-Zeroable plain structs, and
> enums with a zero member are derived structurally; constructor/destructor
> types and other invariant types require an explicit audited
> `unsafe impl Zeroable`. `MaybeUninit<T>` is unconditionally zeroable;
> `ManuallyDrop<T>` requires `T: Zeroable`. `@allow_zeroed` is removed and its
> in-tree uses migrated. `std::vector::resize` now has a `T: Zeroable` bound
> instead of zeroing arbitrary `T`. Production call sites that need an
> uninitialized/zeroed temporary now use the explicit unsafe
> `std::mem.zeroed_unchecked<T>()`; `zeroed:unsafe<T>()` remains only as the
> audited bootstrap inside that helper (and a few generic/array test cases).
> Negative tests cover missing bounds, explicit impls, arrays,
> nested enums, references, and the safe/unsafe boundary.
> **S5.6b policy decision (2026-10-29):** panic always aborts. The runtime has
> one profile; there is no unwinding, drop-on-unwind, `catch_unwind`, or
> `UnwindSafe`/`RefUnwindSafe`/`AssertUnwindSafe` surface. A panic in a
> destructor or a worker thread aborts the process. This also makes FFI unwind
> barriers unnecessary. The decision is revisited only if a future backend
> makes unwinding cheap enough to justify the C-translation complexity.
> **M6 safe threads (first slice):** `std::thread.spawn` submits a
> non-capturing `(T) => U` entry with `T: Send + 'static` and
> `U: Send + 'static`; `std::thread.spawn_void` is the void-returning form.
> `std::thread.JoinHandle<T>` joins on explicit `join()` or destructor and is
> `Send + Sync` when `T: Send`; `std::concurrent.Thread.join` is now public.
> `std::thread.scoped_spawn`/`scoped_spawn_mut` join before returning and accept
> a non-`'static` borrowed value (`T: Sync` shared, `T: Send` mutable), with
> runtime tests for both. A
> non-`Send` argument (`Rc`, raw pointer) is rejected by the existing auto-trait
> checker, and `T: 'static` is now enforced for generic bounds
> (`SAFE-LIFE-004`): a reference into the caller's stack (directly or in a struct
> field) cannot escape into a thread. `vector`, `deque`, `unordered_map`, and
> `ordered_map` gained audited
> conditional `Send`/`Sync` impls; `string`/`u16string` remain owned values.
> `std::thread.scoped_run` adds a multi-handle shared scope: `ScopedScope<T>`
> starts up to 32 workers over one borrowed `T: Sync` value and joins all of
> them from its destructor; the internal fields make the scope unforgeable.
> The `%capture<F, Env>` instance type is the function-instance wrapper, not
> the generated capture environment, so deriving Send/Sync from it would be
> unsound; a concrete typed capture representation is still required. Capturing
> closures that need extra state, heterogeneous multi-value scopes, and erased
> `std::function`/`FutureHandle` captures still use the explicit unsafe
> `std::concurrent.spawn` path until M6.5.
> **M6.2 TSAN:** `scripts/thread-tsan.sh` builds
> `lang/tests/thread_tsan/chemical.mod` with `--tsan` and runs four safe
> threads sharing `Arc<Mutex<int>>` (4000 serialized increments). The bundled
> LLVM prebuild may not contain `libclang_rt.tsan.a`; the script discovers a
> system LLVM resource directory (or honors `CHEMICAL_TSAN_RESOURCES`) and
> fails rather than silently dropping the sanitizer. Guard-transfer negative
> tests cover `MutexGuard` and `RwLockWriteGuard`.
> Callback capture escape: constructing a callback from a lambda that captures
> non-global storage by reference, or captures a reference/view by value,
> requires an unsafe boundary (`SAFE-UNSAFE-002`). Owned by-value captures stay
> safe. `SAFE-LIFE-001` rejects direct/aggregate returns and follows local,
> assignment, struct/array field, access-chain, erased-parameter, and
> variant/generic taint. This closes the safe propagation paths for the erased
> representation. Bare `std::function` parameters still erase their witness by
> type; a signature that must expose `Send` uses `std::typed_function<F, C>`.
> M6.5 first slice: `CapturingFunctionType` carries an optional capture-env
> witness; lambdas get a synthetic environment of their captured field types.
> `Send`/`Sync`/`'static` recurse into the witness, plain `std::function`
> variables acquire it from lambda initializers, and `std::typed_function<F,C>`
> exposes it. Safe `std::thread.spawn_closure`/`spawn_void_closure` infer `C` and
> require `C: Send + 'static`; owned captures run safely, `Rc` and borrowed
> captures are rejected. Safe `async::spawn_send` adds a compiler frame check
> (`SAFE-SEND-001`): a direct async call is accepted only when every frame
> parameter/local is `Send + 'static`; stored/erased futures are rejected.
> The checked result is `core::async::SendFutureHandle<T>`, which carries a
> type-level `Send`/`Sync` witness when `T: Send`, so a submitted future can be
> moved to another thread and driven by `async::block_on_send`. Raw
> `FutureHandle<T>` remains erased/`!Send` outside that boundary. Capturing
> scoped closures are covered by `std::thread.scoped_spawn_closure`: the
> witness may borrow caller storage (`C: Send`, not `'static`) because the
> worker is joined inside the call. Unsafe-created borrowed-callback taint is
> followed through assignments, fields/indexes, access chains, erased callback
> parameters, and variant/generic payloads so it cannot cross a safe API that
> erases the witness. M6 now includes
> join-before-return `std::thread.scoped_spawn`/`scoped_spawn_mut`,
> multi-handle `scoped_run`, and `JoinHandle<T>: Sync` when `T: Send`. M4 is complete for the enforceable language surface: NLL/loans, storage
> lifetimes, return regions, variance, temporary extension, provenance,
> static-lifetime enforcement, Pin/Waker/async-frame checks, and HRTB call
> regions are implemented. HRTB retaining storage now accepts caller-owned
> slots when the caller's `where` edges prove every source outlives the stored
> region; the slot's outer borrow only has to cover the call, so an unrelated
> outer region is no longer rejected when the pointee and source already share
> their declared region. Pointees using different higher-ranked binders are
> accepted because the call site checks every source against every stored
> region; `'static` pointees and free pointee lifetimes carried by a direct
> reference source are accepted, while opaque retaining storage is now
> fail-closed. HRTB parameters support nested struct
> aggregates, variant payloads, and array elements, borrowed nested results
> through path-aware MIR field-source mapping, and per-leaf multi-lifetime
> precision. Large reference arrays are supported at the top level and as one
> direct struct field. Multi-borrow HRTB calls accept mixed
> local/parameter/global referents and distinct caller regions when the
> callback has no retaining storage. A retaining parameter whose own storage
> is local or a by-value copy also accepts mixed/distinct sources; a
> caller-owned reference slot requires the source-to-stored proof.
> Scoped threads without captures are covered by join-before-return
> `scoped_spawn`/`scoped_spawn_mut` and the multi-handle shared `scoped_run`
> scope, including non-`'static` borrows. Capturing scoped callbacks are covered
> by `std::thread.scoped_spawn_closure` for the no-argument captured form.
> `std::thread.scoped_run_closure` / `ScopedClosureScope` cover heterogeneous
> multi-value scopes (distinct capture witnesses in one join-all scope), and
> the value-passing forms (`scoped_spawn_closure_value`/`_mut` plus scope
> `scoped_spawn_closure_value_in`/`_mut_in`) pass a borrowed value to a
> captured closure. Nested closures resolve captures through the enclosing
> closure environment, and generic callback parameters are deferred to
> instantiation. `std::send_function<T>` keeps a verified `Send + 'static`
> witness in a non-generic parameter (`SAFE-SEND-002` rejects erased, `Rc`,
> and borrowed witnesses), so APIs need not expose `C` when only the thread
> guarantee matters. `std::scoped_send_function<T>` proves `Send` without
> `'static` and is itself `!Send`; a `@scoped_callback` function may forward it
> only to a join-before-return scoped API, which lets a bare borrowed-capture
> parameter be constructed without `unsafe`. Bare `std::function` parameters
> still erase; use `send_function`, `scoped_send_function`, or
> `typed_function`. Raw `FutureHandle<T>` remains
> intentionally `!Send`; safe submission returns `SendFutureHandle<T>`. Packed layout and `@repr("C"/"packed")` are implemented
> (including `SAFE-VALID-008` for references to packed fields);
> `@repr("transparent")` is implemented for checked single-field structs
> (layout and C ABI); C ABI classification for Windows/ARM, FFI wrappers, and
> deeper HRTB precision remain (variance/temporary-lifetime extension are
> implemented).
> Safety suite `64`; negative tests `702`; main TCC `2378`, LLVM `2376`;
> libs `730`; plugins `1305`; async `69`; interpret `1910`; server `10`;
> TLS `599`; `MIRTests` `4643 checks`. The whole-tree audit
> manifest (`lang/docs/safety/audit-modules.txt`) covers `94` modules with
> `unsafe_blocks 3774` / `unsafe_values 4901` (impl bodies included; see
> `lang/docs/safety/migration-metrics.md`).
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
| Runtime model | Bounds checks always on; integer overflow checked in debug and wrapping in release; division/shift always checked; failures panic. Checked/wrapping/saturating APIs available. Panic aborts; unwinding/`catch_unwind` are not part of the current target. |
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
5. **Safety-relevant parity.** Every feature and shipped safe API needed to
   express sound code is implemented and tested on TCC and LLVM; the interpreter
   runs every feature it supports, and unsupported coroutine/interface opcodes
   are hard diagnostics. Interpreter coverage gaps are release blockers for the
   interpreter corpus (M5.7/M8.1), not silent skips. Feature-parity-only items
   (GATs, const generics, built-in derive, `impl Trait`/RPITIT, default-method
   parameter projections, match/switch-position patterns, tuple trait impls,
   raw unsized `[T]`, full UTF-8 surface) are not release blockers unless a
   concrete safe API requires them; missing safe wrappers and unaudited
   `unsafe` are still release blockers.
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
- `panic` is safe and aborts. There is one profile; no unwinding,
  `catch_unwind`, or drop-on-unwind.
- Bounds, debug overflow, division by zero, invalid shifts, failed
  `RefCell`/`Mutex` borrows panic.
- Panicking in `Drop` aborts immediately; there is no unwinding state.
- The panic effect (proposal 0005) is implemented as a first slice: a function
  may declare `[no_panic]` after its signature. Its lowered MIR is rejected if
  it calls a callee that is not explicitly `[no_panic]`, calls through a
  function pointer/dynamic dispatch, throws, or emits a runtime panic call
  (bounds/overflow/division/allocation failure). `unreachable` is an unsafe
  trap, not a panic, so `[no_panic]` does not reject it. Only the `panic` effect
  is parsed today; `[no_alloc]`, `[no_unsafe]`, `[no_blocking]`, `[pure]`,
  module defaults, and effect propagation through generic/async bodies remain
  future proposal-0005 work.

### 3.5 GC boundary

Owned and GC code do not mix implicitly. GC modules relax lifetime/UAF checks
but keep aliasing/race checks. GC is a later mode.

---

## 4. Current-state audit

### 4.1 Implemented surface (2026-10-31)

| Mechanism | State | Notes |
|---|---|---|
| Destructors/RAII | Implemented | `@delete`, member destructors, MIR `Drop`/`Destroy`, per-path drop flags. |
| Move/init tracking | Implemented in MIR | `MIRMoveCheck` is the sole init/drop producer; partial struct/array init modeled. Field-wise drop elaboration for partial **array** and non-`Copy` struct-field moves remains. |
| `Copy` | Opt-in and enforced | `SAFE-COPY-001/002/003`; primitives/pointers/references/functions/arrays built-in. |
| `Clone`/`Default` | Manual bounds | Automatic `Clone` derivation remains M1 work. |
| References/lifetimes | Enforced for supported surface | `MIRRegionSignature`, NLL loans, storage lifetimes, return regions, variance, temporary extension, `'static`, HRTB call regions. |
| `unsafe` | Enforced | Raw pointer/reference/asm/extern/global boundaries; named escape flags removed; `unsafe` never disables move/borrow/init checks. |
| FFI | Enforced calls; target-aware C ABI | `SAFE-FFI-001/002/003/004/005`; by-value structs/unions on x86-64 SysV, Windows x64/ARM64, i386 SysV/Windows, ARM32 AAPCS (soft/hard-float) and AAPCS64; unknown targets reject by-value extern aggregates and variadic aggregate arguments are rejected instead of emitting a guessed shape. |
| Runtime checks | Implemented | Array/vector/deque/string/span/map bounds, div/mod/shift, checked signed arithmetic, defined float→int, allocation/layout guards, abort-only panic. |
| Thread safety | Enforced safe surface | `Send`/`Sync` auto traits, explicit/negative impls, nested conditional generic impls, scoped/owned thread APIs, `Arc`/channel audit, TSAN smoke. |
| Async | Shipped; type-level lifetime + field/container inference | `Pin`/Waker/frame loans; `spawn_send` frame check; MIR future borrow tracking through fields/wrappers/pointer containers + `SAFE-LIFE-005` retaining-API rejection. |
| Trait objects | Partial | Basic dynamic dispatch; `dyn I + Send`/`+ Sync`/`+ Unpin` are represented and construction-checked, and `dyn` without bounds is conservatively `!Send`/`!Sync`/`!Unpin`. |
| Tuples | Implemented slice | Construction, `._0` access, parameters/returns, nested tuples, assignment, structural `Copy`/`Send`/`Sync`/`Unpin`, field-wise drops, flat and nested destructuring with wildcards (`var ((a, b), _) = value`), tuple patterns in `if` conditions, named/generic tuple structs, inline and external inherent methods on tuple structs (including generic ones), unit tuple `()`, flat and nested fixed-array patterns (`var [a, b] = arr`, `var [[a, b], [c, d]] = matrix`, `[first, .., last]`, `_`; Copy elements copy, non-Copy owned locals move through the full constant-index group path; nested groups are exact), and exact slice-view condition patterns (`if(var [a, b] = view)`). Match/switch patterns, `impl Trait for (T1, T2)`, and raw slice rest forms remain. |
| Slices / `&str` | Borrowed, mutable, and owning slices | `[T]`/`&[T]` alias `std::span<T>`; `&mut [T]` aliases `std::mut_span<T>`; `Box<[T]>` aliases `std::boxed_slice<T>`; `&str` aliases UTF-8-validated `std::str`. Indexing, writes, owning moves, local-escape checks, and the `IndexMut` interface are implemented. Fixed-array patterns, exact/rest condition patterns, and runtime-checked statement patterns with divergent `else` branches for `[T]`/`&[T]`, `&mut [T]`, and `Box<[T]>` are implemented; raw unsized `[T]` remains. |
| Associated types | Declarations + bindings + static projections + equality bounds | Interface `type Item` declarations, impl `type Item = T` bindings, implementation completeness, return/abstract-parameter `Self::Item`, and `T::Item` through inline/`where`/inherited bounds are implemented. Generic struct fields, nested generic containers, generic impl bindings that project a type parameter, `type Item: Bound` checks, and `Interface<Item = T>` equality constraints (inline and `where`, concrete instantiation-checked, inherited-interface-aware) are covered. Default-method parameter projections are rejected; dyn projections and GAT parameters remain M1. |
| Const generics/GATs/`impl Trait` | Absent | Const generics, GAT projections, and `impl Trait` remain M1. |
| `&str`/UTF-8 | Byte-string surface | `string`/`string_view`; UTF-8 parity M1. |
| Derive | Absent | CBI macros exist; no built-in derive. |
| Macros/plugins | Generated Chemical checked | Raw generated JS stays outside native safety. |

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

### 4.3 Historical checks and their replacements

| Old check | Replacement |
|---|---|
| Definite assignment in `TypeVerify` | `MIRMoveCheck` (init/move/drop dataflow). |
| Destructible deref/index in `TypeVerify`/`SymResLinkBody` | Borrow rules + raw-pointer `SAFE-UNSAFE-001`. |
| Temporary lifetime escape in `VisitFunctionCall` | Region/lifetime analysis + `SAFE-LIFE-001/002/003`. |
| Move tracking in `SymResLinkBody` | MIR move/init checker. |
| `unsafe` deref warning | Enforced `SAFE-UNSAFE-001/002/003`. |
| `@unsafe` alias | `unsafe func`; `unsafe(...)`/`unsafe {}` contexts. |
| `unsafe "no_lifetime_check"` | Removed; named flags are not semantic escapes. |
| `@allow_zeroed`/`zeroed:unsafe` | `unsafe impl Zeroable` + `std::mem.zeroed_unchecked`. |
| Module `safety`/`safe_mode`/`checks.*` | Removed; no user opt-out. |

### 4.4 Baseline (2026-10-31 audit)

| Gate | Result |
|---|---|
| TCC main / LLVM main | `2419` / `2417` |
| `@test` runner | `56` |
| TCC/LLVM negative | `727` / `727` |
| TCC/LLVM safety | `74` / `74` |
| TCC/LLVM async | `69` / `69` |
| TCC libs | `730` |
| TCC plugins | `1305` |
| TCC interpret | `1921` |
| `MIRTests` | `4682 checks` |
| TSAN smoke | pass (`--llvm --thread-tsan`) |
| Whole-tree audit | `unsafe_blocks 3775`, `unsafe_values 4912`, `raw_pointer_types 9370`, `raw_pointer_derefs 1071`, `panic_calls 112` |

The audit count is higher than the historical `3661/4890` because the report
previously skipped **all** `ImplDefinition` bodies; it now walks them, and the
new async-closure fixtures exercise the documented raw C future ABI. Treat the
new number as the honest baseline, not a compiler regression.

### 4.5 Known remaining gaps / release blockers

1. **`FutureHandle<'a, T>` carries a frame lifetime; full inference remains.**
   Generated async returns use `'static` for owning frames and an elided
   lifetime when a parameter/frame value borrows. A borrowed handle cannot
   satisfy a `FutureHandle<'static, T>` slot; `spawn_send` requires `'static`,
   and a return to a legacy lifetime-less `FutureHandle<T>` from a borrowed
   future is rejected. MIR tracks direct frame borrows and `SAFE-LIFE-005`
   rejects a future (or a struct/array/`Option` containing one) passed to a safe
   callee that is not a known consumer and does not return a future-containing
   result. Direct returns, locals, wrappers, methods, struct fields, `Option`,
   `string_view`, safe stash APIs, safe references, and callback captures are
   covered. `SAFE-LIFE-005` follows references (`&FutureHandle`) and the real
   lambda capture list, including an erased `std::function` field/argument
   (including a callback assigned after declaration through the taint set), so a
   borrowed future cannot be hidden behind a reference or callback capture.
   MIR loan propagation follows a future-containing aggregate returned by a
   pass-through wrapper (`fn wrap(h) -> W`), distinguishes the declared return
   region of a wrapper from the region it actually returns (`SAFE-LIFE-003`),
   and seeds caller-owned loans for reference parameters of future-returning
   functions, including elided reference parameters. A safe custom consumer is
   accepted when its body only forwards the handle to a known consumer or
   another checked consumer; storing/returning/capturing it remains rejected,
   and a consumer body containing an unsafe block is not trusted.
   Aggregate field loans are lifted when a future-bearing value is passed by
   value (`wrap(w)` / `identity(w)`), `Box`/`Rc`/`Arc` pointer containers are
   followed by MIR, and whole-place extraction through `Option::take` /
   `Box::into_inner` carries the source loan. Custom consumers may alias the
   handle through a local (`var local = h; block_on(local)`), but not through a
   field/return/capture. Owning `FutureHandle<'static, T>` captures are no longer
   treated as borrowed. Function-type call contracts now carry free (non-HRTB)
   lifetimes from parameters/returns and generic arguments; a function value of
   a lifetime-generic declaration is treated as higher-ranked, while a call
   through a free-lifetime contract keeps the conservative escape mark. A
   callback returning `FutureHandle` must tie its frame lifetime to a reference
   or future parameter (or use `'static`); higher-ranked `FutureHandle` returns
   are supported, and capture-owning callbacks whose result has a free/legacy
   lifetime are rejected at the call site because the erased capture environment
   may die before the future. Nested aggregate returns carry their future
   lifetime through `Outer<'a> -> Inner<'a> -> FutureHandle<'a>`, and the MIR
   call contract uses the nested future's lifetime rather than an unrelated
   aggregate argument. A safe call returning a type-erased generic container
   (`Opaque<FutureHandle<'a>>` with a `*mut void` field) is rejected with
   `SAFE-LIFE-005` when the representation does not expose the future in a
   storage field; `Outer<'a> -> Inner<'a>` wrappers with a real future field
   remain inspectable, while `Holder<T>`-style wrappers that erase the nested
   future's lifetime are rejected. Non-lifetime wrappers, type aliases, and
   `Holder<FutureHandle<int>>`-style erasures are covered by negative tests.
   Remaining: user type erasure performed entirely inside an explicit `unsafe`
   boundary, which cannot be distinguished from other audited unsafe code.
2. **Target-aware C ABI is implemented for the main native conventions.**
   `MIRABI.h` classifies by-value structs/unions for x86-64 System V, Windows
   x64, i386 System V, Windows i386, ARM32 AAPCS (soft-float and
   hard-float/AAPCS-VFP), AArch64 AAPCS64 and Microsoft ARM64. Classification is
   parameter/return-aware: i386 Windows and ARM32 return small aggregates in
   registers while passing parameters differently; ARM32 hard-float handles
   HFAs and non-HFA parameters use Clang's 64-byte split/byval threshold;
   Microsoft ARM64 uses HFA FP parts and x0/x1 for non-HFA composites up to
   16 bytes. LLVM converts register parts at the exact aggregate offset; memory
   parameters use `byval` on SysV x86, i386 Windows and ARM32, and a plain
   pointer on Windows x64/ARM64/AArch64, matching Clang. Unknown targets reject
   by-value `@extern` aggregates with `SAFE-FFI-003`; the C backend still
   delegates to the host C compiler. By-value aggregates passed through a
   variadic `@extern` call are rejected with `SAFE-FFI-004` (pointer/scalar
   rewrite required). `SAFE-FFI-005` audits callback signatures: by-value
   aggregate callback parameters/returns and capturing callbacks are rejected.
   Scalar/pointer cdecl and explicit `@stdcall` callbacks are accepted on 32-bit
   x86; `@stdcall` is part of function-type identity and flows through MIR and
   both backends. `SAFE-FFI-006` keeps variadic `@stdcall` and non-x86 32-bit
   targets fail-closed; LLVM now emits `x86_stdcallcc` for direct i386
   Linux/Windows symbols and derives the Windows COFF `_name@N` decoration
   from the function type. `cstd/windows` is safety-clean on an i386 Windows
   target. Tranches 1-3 of the semantic wrapper audit are complete: allocator
   ownership/zero-size, filesystem path/file null and capacity checks, socket
   buffer/length/port validation, zero-sized `Box` allocation, `DynBox` field
   visibility, string append null validation, zero-sized vector support,
   `Rc`/`Arc` count-overflow checks, unsafe-gated `u16_strlen`, thread-pool
   `Future` ownership/null safety, vector replacement drops, and filesystem
   temp/copy path buffer bounds, non-`Copy` map `find` rejection, and
   worker-owned thread-pool teardown. See
   `lang/docs/safety/ffi-wrapper-audit.md`. Item 1's wrapper audit now covers
   the claimed Linux, Windows, macOS and FreeBSD roots; remaining target-source
   work is limited to non-claimed OS mappings and the Linux/Windows-only
   `window`/`webview` backends. `string_view`/`u16string` raw constructors,
   `Rc`/`Arc` weak handles, and `RefCell` guard escape were reviewed as
   review-only items; no memory-safety gap was found.
3. **`@scoped_callback` is body-checked, including custom sinks.**
   `SAFE-SCOPED-001` rejects returning, storing, capturing, or passing the
   scoped parameter to an unknown callee. Known join-before-return APIs
   (resolved by exact library namespace) and other annotated functions are
   accepted directly. A custom sink is accepted only when its body is visible
   and the same forwarding proof succeeds recursively: it may discard the
   callback or forward it only to another checked sink. A sink with no body,
   a recursive cycle, or a body that stores/returns/captures the callback is
   rejected.
4. **`Fn`/`FnMut`/`FnOnce` callable kinds are implemented for fixed and variadic
   function signatures.** `std::Fn<T>` is the reusable callable type;
   `std::FnMut<T>` requires a mutable local/field receiver; `std::FnOnce<T>`
   consumes the callback after its first call (`SAFE-MOVE-003` plus the MIR
   destroyed-state check). A `FnOnce` lambda may move a by-value capture out into
   an explicit `FnOnce` target; per-capture drop flags prevent double-dropping
   moved fields. Field-stored `FnOnce` calls move the field into a temporary and
   zero the source, with a runtime null-callback abort on reuse. `core::ops::Fn`/
   `FnMut`/`FnOnce` marker bounds are enforced for fixed and variadic signatures,
   and generic call syntax through all three bounds is lowered, including
   capturing lambdas and explicit or non-capturing `FnOnce` callbacks. Untyped
   capturing lambda initializers infer a callable type and upgrade to `FnOnce`
   when their body moves a capture. Capturing async closures are lowered as
   `FnOnce` and frame-own their function instance/capture allocation; a
   borrowed capture cannot be typed as `FutureHandle<'static, T>`.
5. **HRTB precision.** Read-only multi-borrow callbacks may mix local,
   parameter, and global referents, and may use distinct caller regions, when
   no parameter exposes storage that can retain a reference. Multi-borrow
   callbacks whose parameters can retain a borrow now check the retaining
   storage location. A local/temporary or by-value parameter copy may retain
   any argument borrow, so mixed local/parameter/global sources with distinct
   caller regions are accepted and all loans are pinned to function exit.
   Retaining storage reached through a reference parameter is caller-owned.
   It is accepted when every source outlives the region stored in the slot's
   pointee (for example `where 'i: 's` for `slot: &'o mut &'s T, x: &'i T`).
   The slot's outer borrow (`'o`) only has to cover the call, so an unrelated
   outer region is accepted when the pointee and source already share their
   declared region; without a source-to-stored edge it still requires one
   shared region. A retaining pointee is accepted when every reference inside
   it uses one of the callback's binders (`&'a mut &'a T` or
   `&'a mut &'b T`), `'static`, or a free lifetime that is also a direct
   reference parameter (so the call contract can map the source loan); the
   call-site rule checks every source against every stored region, so distinct
   binders do not weaken the proof. An opaque retaining type with no named
   stored region is rejected for a caller-owned slot instead of being accepted
   by the equal-region shortcut. Nested struct aggregate
   parameters whose leaves are direct reference fields are accepted
   recursively, and a borrowed nested-field result keeps the aggregate's
   source loans alive through the MIR field-source chain. Nested fields now
   carry their full root-to-leaf path and their own declared lifetime, so
   multi-lifetime aggregates (`Pair<'a, 'b>`) keep precise per-leaf loans.
   Variant payloads and constant array elements are part of the same
   root-to-leaf path, so a borrowed payload/element result keeps its source
   loan live. Fixed arrays up to 64 elements are path-mapped per element; a
   **top-level** larger array of direct references is described by one
   wildcard element step and one element-loan region, so the result keeps its
   source loan live (its elements still share the element lifetime and remain
   conservatively aliased). A large direct-reference array now uses the same
   wildcard leaf at any aggregate depth, including a struct field, a nested
   struct field, and an array-of-structs element; the MIR signature records the
   full root-to-leaf path and the borrow checker keys the wildcard loan by the
   owning root and leaf field index. Each element/leaf still shares its
   element lifetime and remains conservatively aliased, and two same-index
   wildcard leaves under one root share one loan rather than guessing distinct
   regions. Unsized arrays keep an explicit `SAFE-BORROW-099` rejection.
   Caller-owned retaining storage without a provable source-to-stored edge and
   opaque retaining storage with no named region remain the HRTB precision
   limit.
6. **`dyn` object model is implemented for the safe surface.** `dyn I` is a
   target-aware two-word borrowed fat pointer (`{ data, vtable }`); `&dyn I`
   dispatches through the same vtable, including through `Box::get`. `Box<dyn I>`
   is a generic `Box` holding a heap fat-pointer **record**, not a Rust-style
   owning trait object: it owns the two words, not the referent. MIR tracks the
   boxed view as a borrow, so returning it from a local is `SAFE-LIFE-001` and
   letting it outlive the referent is `SAFE-LIFE-002`. `std::DynBox<I>` /
   `std::dyn_box<T, I>` provides the owning erased value: it owns one concrete
   allocation and drops it exactly once through a stored `drop_erased<T>`
   function. `dyn Derived` upcasts to `dyn Base` when Base is in the
   first-inheritance chain (vtable slots are a prefix). `dyn I + Send` and
   `dyn I + Sync` are represented and checked against the concrete value at
   construction (`SAFE-SEND-003`); `+ Unpin` is checked with the same
   PhantomPinned negative-impl path, and unbound `dyn` remains
   `!Send`/`!Sync`/`!Unpin`. Object safety now also requires every interface
   method to have a self receiver and rejects `Self` nested in parameters or
   returns (for example a static/zero-receiver method). `Box<dyn I>`/`Arc<dyn I>`
   can cross threads only with the explicit bound. See
   `lang/docs/dyn-object-model.md`; M1.19.
7. **Async allocation checks are current, not permanently complete.**
   `yield.ch`, `time.ch`, `exec.ch`, `blocking.ch`, `combinators.ch`,
   `reactor.ch`, and `frame.ch` now check allocation failure before writing.
   New constructors must follow the same checked/`new` pattern; broader stdlib
   allocation-failure coverage is M7. The executor's public raw-state entry
   points (`exec_make_waker`, `executor_set_wake_hook`, `exec_enqueue_task`,
   `executor_poll_tasks`, `executor_wait`) are now explicit `unsafe func`s,
   reject null state before use, and skip malformed queued tasks. Runtime state
   structs (`TaskVTable`, `RawTask`, `Executor`, `JoinState`, `SpawnTask`,
   `SelectState`, `TimeoutOrState`, `BlockingState`) carry explicit
   `!Send`/`!Sync` impls so a representation refactor cannot silently make them
   transferable. `window` wraps the raw executor calls in audited unsafe blocks.
   TSAN coverage now includes `Mutex`, `RwLock`, a cross-thread channel handoff,
   thread-pool task submission, and an executor `spawn_send`/`yield` handoff.
   Elided/named-lifetime `span`/`mut_span`/`str`/`string_view` remain
   conservatively non-transferable; their explicit `'static` forms are now
   `Send`/`Sync` (element rules apply). Remaining async coverage is the
   executor-state false negatives and the type-level `FutureHandle` erasure
   boundary.
8. **The audit gate is CI-wired; platform breadth is not complete.**
   `.github/workflows/safety-audit.yml` runs `scripts/safety-audit.sh` on PRs
   and `main`. The 94-entry list is host-platform-oriented. The i386 Windows
   target is safety-clean for `cstd`, `std`, `fs`, `net`, `test`, `async`,
   `tls`, `http`, `server`, `window`, and `webview`; the macOS and FreeBSD
   roots (including `http`/`server`) are now checked by strict `--target
   --check` regressions. Remaining platform work is non-claimed OS mappings
   (`NetBSD`/`OpenBSD`) and the Linux/Windows-only windowing/webview backends;
   `--update` is reviewed manually.
9. **Audit reporting precision.** Impl bodies are now counted, and
   `unsafe func` / `unsafe impl` declarations are tracked as separate
   `unsafe_func_decls` / `unsafe_impl_decls` metrics. Generic master/concrete
   impls can still double-count syntax, and the module list remains
   host-platform-oriented; non-Windows OS-gated sources still need
   target-specific audit entries.
10. **Partial-array and non-`Copy` struct-field drop elaboration.** Local,
    by-value-parameter, and async-frame aggregates with generated drop glue carry
    one drop flag per destructible field/element. Nested structs and fixed arrays
    are tracked recursively. Fixed arrays with at most 64 destructible elements
    get one scalar flag per element. Larger fixed arrays of **directly**
    destructible elements use one runtime bool-flag array per element, and
    larger arrays of generated-glue aggregates use one bool-flag array per
    leaf field with an element template, so constant-index whole-element and
    per-field initialization/replacement keep working past the old 64-element
    limit. Only initialized leaves are dropped; a partially initialized sibling
    field is never passed to its destructor. Nested fixed arrays inside a
    template element are tracked recursively up to 1024 inner elements: each
    inner leaf gets its own outer-index flag array, and inner element addresses
    are offset from the template base. A small outer array whose element is
    itself a large array uses one flag block per element; whole-element
    assignment/destruction arms or drops the whole inner block. Directly
    destructible large arrays in async frame slots now use a frame-resident
    bool-flag block, so partial initialization across suspension no longer
    leaks or drops uninitialized slots. Generated-glue array slots in frames use
    an element template with frame-resident leaf flag blocks. Inner arrays are
    tracked recursively; an inner array above 1024 uses one flat flag block
    (`inner_count`) with a runtime inner loop instead of expanding one child per
    index. Unsized arrays still fall back to a whole-field flag.
    Whole-field construction/destruction of a generated-glue large array field
    inside a frame aggregate is covered by the same template. A constant-index
    write to an element of a tracked array field (`h.items[0] = ...`) now
    initializes that element and its field leaves. A nested constant-index
    element write (`cube[a][b][c] = ...`) resolves the full index chain through
    scalar inner flags, whole-child trees, large flat inner flag blocks, and
    element templates.
    Whole-array field moves are covered. Moving a field clears its flag,
    so the parent destructor cannot double-drop it. Async frame flags are
    frame-resident and are used by normal completion, cancellation, and resume
    cleanup; a moved frame field no longer rejects with `SAFE-MOVE-004`.
    A constant-index move out of an owned local fixed array (`var e = arr[0]`,
    `return arr[0]`, a by-value argument, or an aggregate initializer) now
    clears that element/leaf flag and emits the MIR move marker. Non-Copy
    fixed-array destructuring uses the same element moves for owned locals.
    A fixed-array field of a mutable local/async-frame aggregate
    (`s.items[0]`, `s.inner.items[0]`) uses the same element move with its field
    path, so its per-element flag under the struct drop tree is cleared.
    A dynamic final index move (`var x = arr[i]`) from a fully initialized
    large array clears the runtime element flag and emits `MoveOut`; the move
    checker conservatively treats the index as the owning root path. Small
    arrays of directly destructible elements select the runtime scalar flag.
    A partially initialized array is accepted for directly destructible
    elements because lowering checks the selected element's flag first and
    aborts through `__chc_uninit_fail` if it is not live; a wholly uninitialized
    array still rejects with `SAFE-INIT-001`. Small arrays of generated-glue
    elements select the element row at runtime and clear every leaf flag, so a
    move from a fully initialized small array works. A partially initialized
    generated-glue array still rejects with `SAFE-MOVE-001` because the runtime
    check would have to prove every leaf live. A small 2D array of directly
    destructible elements supports two runtime indices (`arr[i][j]`), including
    a constant prefix before the pair (`arr[0][i][j]`), through a flat
    pointer table that covers every small/small shape (the 64 x 64 maximum is
    4096 flags). The same selector serves writes and moves, including a runtime
    outer index with a constant inner index (`arr[i][0]`); partial moves use
    the runtime flag check. A runtime index before a constant middle index
    (`arr[i][0][j]`) and three runtime indices (`arr[i][j][k]`) are supported
    for small directly destructible arrays. Expanded small-array rows also
    support arbitrary runtime depth (`arr[i][j][k][l]`, 5D, ...) and runtime
    values mixed with constant indices; generated-glue 3D elements use the same
    runtime tree, including `arr[i][CONST][j]` and `arr[CONST][i][j]`.
    Generated-glue 2D arrays now support small/small, outer-small/inner-large,
    outer-large/inner-small, and both-large layouts; both-large leaf flags are
    outer-indexed per inner row up to 1024, and a larger inner dimension uses a
    flat `outer * inner + inner` block. Constant writes and moves use the full
    index chain. Mixed dimensions are supported: an
    outer-small/inner-large array selects the outer row's flat flag block,
    while an outer-large/inner-small array selects the inner element's
    outer-indexed flag array. Both dimensions large use the outer element
    template's per-inner flag arrays, or one flat inner block when the inner
    dimension exceeds 1024. Generated-glue 3D arrays use the same runtime tree
    for expanded dimensions and the compressed selectors when a dimension is
    large: inner-large (`[a][b][>64]`), middle-large (`[a][>64][b]`), and
    outer-large (`[>64][b][c]`) templates are selected by runtime index, with
    mixed constant/runtime chains. Middle-large and outer-large rows support
    both expanded inner rows and flat inner blocks; inner-large templates
    support generated leaf flag arrays. A generated inner array above 1024 under
    a large outer/middle template now uses one flat flag block per **direct
    leaf field**, with the element type recorded so scope destruction loops the
    inner elements and drops each leaf. Generated elements whose destructible
    leaves are nested paths still fall back to the legacy one-flag-per-element
    block and remain reject-only for per-field selection. A scalar read
    through a
    runtime-selected direct element (`arr[i].field`, `arr[i][j].field`, or a
    field array) checks the element flag first and marks the derived field
    address with `VF_INIT_CHECKED`, so the move checker accepts the guarded
    `Load`. Reads of generated-glue elements check every leaf of the selected
    element (a runtime row branch for small arrays, indexed flag arrays for
    templates). Constant-index aggregate/method reads resolve all constant
    indices and check every tracked leaf, closing the method-receiver path that
    never passes through a MIR `Load`. Normal non-dynamic scalar field loads
    keep their original provenance.
    `const` bindings, array parameters (passed as pointers), array fields
    reached through references/pointers, and borrowed/slice views remain
    rejected with `SAFE-MOVE-004` rather than being guessed safe.
    Non-Copy destructuring of owned local arrays follows the full
    constant-index path for expanded small arrays, while unsupported
    large/template or borrowed shapes remain `SAFE-MOVE-004`.
    A dynamic final index write into a large array (`arr[i] = value`) resolves
    the flat element flag or element template, destroys the old element only
    when its flag is live, and arms the new element. It is also allowed when
    the array is partially or wholly uninitialized: the runtime flag records
    the live element, the array enters the partially-initialized state, and
    whole-array reads/moves still require full static initialization. Small
    arrays of directly destructible elements select the scalar element flag at
    runtime; small arrays of generated-glue elements select the element row and
    arm every leaf flag, so dynamic writes into initialized or uninitialized
    small arrays work. Field array writes through dynamic indices are likewise
    tracked. The move
    checker now distinguishes `PartiallyInitialized` from `PartiallyMoved`:
    the former keeps whole-value reads/moves at `SAFE-INIT-001`, while a
    runtime-indexed move of a directly destructible element can proceed after
    its flag check. A temporary aggregate field array
    (`make().items[CONST]`) is materialized as a block-scope destructible like a
    temporary field move. A direct temporary array value (`make()[CONST]`) is
    also accepted now that the C backend copies array-valued sret returns with
    `memcpy` instead of emitting an illegal array assignment. A field move from
    a materialized temporary/expression
    (`make().field`) is supported when the lowered field address has a
    temporary root with generated field drops: the temporary is registered as
    a block-scope destructible, the moved field flag is cleared, and the
    remainder is dropped at block end. User-destructor temporaries and
    pointer/dereference bases remain rejected. Moves from a user-destructor
    aggregate, a global, or a reference/pointer remain rejected.
11. **Interpreter** reports async/coroutine opcodes, inline asm, dynamic
    call/vtables and `Unreachable`/`Throw` as explicit hard errors; it does not
    silently execute them. Bounds/provenance/tombstones are implemented.
12. **Panic unwinding is withdrawn by policy.** No `catch_unwind`, no
    `UnwindSafe`, no drop-on-unwind.

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
| Core traits | `PartialEq`, `Eq`, `PartialOrd`, `Ord`, `Hash`/`Hasher`, `Display`, `Debug`, `From`/`Into`/`TryFrom`/`TryInto`, `Borrow`, `ToOwned`, `Error`, `FromIterator`/`Extend`, `DoubleEndedIterator`/`ExactSizeIterator` (`UnwindSafe` deferred with unwinding) |
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
| Panic | `panic`, abort-only profile, source/reason reporting; unwinding, `catch_unwind`, and `UnwindSafe` are deferred |
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
  Panic aborts: no destructors run during a panic; normal-path drops are unchanged.

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
  (UnwindSafe traits are deferred with unwinding.)
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
- Cancellation/drop runs exactly once on the normal (non-panicking) path; a panic aborts instead of unwinding.

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
- `SAFE-LIFE-004`: a type does not satisfy a `'static` bound or a `'static`
  lifetime argument (for example, a stack reference submitted to a thread or
  stored in `vector<G<'static>>`).
- `SAFE-LIFE-005`: a future-containing value is passed to a safe callee that
  is neither a known consumer nor returns a future-containing result.
- `SAFE-SCOPED-001`: an `@scoped_callback` parameter is used outside a known
  join-before-return scoped sink.
- `SAFE-FFI-003`: LLVM by-value `@extern` aggregate on a target without a C ABI
  classifier (currently RISC-V, WASM and other unknown targets).
- `SAFE-FFI-004`: a by-value aggregate is passed through a variadic `@extern`
  call; C default promotions cover scalars, not a guessed aggregate layout.
- `SAFE-FFI-005`: an `@extern` callback has a by-value aggregate parameter or
  return, or is a capturing function. Scalar/pointer cdecl and explicit
  `@stdcall` callbacks are accepted.
- `SAFE-FFI-006`: variadic `@stdcall` or a non-x86 32-bit `@stdcall` target;
  use the C backend or remove the annotation. Direct x86 32-bit stdcall is
  supported.
- `SAFE-MOVE-003`: a `FnOnce` callback is called through a reference, or a
  `FnMut` callback is called through a `const` local or an immutable receiver
  chain/field.
- `SAFE-BORROW-099`: an unsupported or unproven borrow/lifetime pattern. The
  current HRTB implementation uses it for retaining storage in a caller-owned
  reference parameter without a provable source-to-stored edge, opaque
  retaining storage with no named region, a free pointee lifetime with no
  direct reference source, and other unproven shapes.
- IDs: `SAFE-OWN-*`, `SAFE-BORROW-*`, `SAFE-LIFE-*`, `SAFE-INIT-*`,
  `SAFE-VALID-*`, `SAFE-BOUNDS-*`, `SAFE-OVERFLOW-*`, `SAFE-SEND-*`,
  `SAFE-UNSAFE-*`, `SAFE-FFI-*`, `SAFE-ASYNC-*`, `SAFE-SCOPED-*`,
  `SAFE-DEP-*`.

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
- No partial moves out of `Drop`; dynamic-index moves are limited to a fully initialized root with a runtime-selectable element flag/template.
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

### 8.6 Panic/runtime ABI (abort-only)

- Every program, including a bare module, links `__chc_panic`,
  `__chc_bounds_fail`, `__chc_overflow_fail`, `__chc_div_zero_fail`,
  `__chc_shift_fail`, and `__chc_alloc_fail` with source/message.
- There is one profile: panic aborts. There is no unwinding runtime,
  drop-on-unwind, `catch_unwind`, or `UnwindSafe`/`RefUnwindSafe`/
  `AssertUnwindSafe` support.
- A panic in `Drop` or in a worker thread aborts the process.
- No unwinding can cross FFI because no unwind exists.
- `std::panic` is a convenience wrapper over the abort entry, not a recovery
  mechanism. Revisit only if a future backend makes unwinding practical.

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
- **Target-aware C ABI**: by-value aggregates/returns and unions are classified
  for SysV x86-64, Win64, WinARM64, i386 SysV/Windows, ARM32 AAPCS
  soft/hard-float and AAPCS64; unknown targets reject. Variadic by-value
  aggregate arguments reject with `SAFE-FFI-004`. Extern callbacks with
  by-value aggregate parameters/returns or capturing functions reject with
  `SAFE-FFI-005`; scalar/pointer cdecl and explicit `@stdcall` callbacks are
  accepted. `SAFE-FFI-006` rejects variadic and non-x86 32-bit `@stdcall`; LLVM
  emits `x86_stdcallcc` (and COFF `_name@N` decoration) for supported i386
  symbols. Remaining here: bitfields (if supported) and the rest of the semantic
  FFI wrapper audit (`lang/docs/safety/ffi-wrapper-audit.md`).
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
- Runtime-fail tests run under the abort profile; expected exit
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
| S1.5 | Auto traits/negative impls (`Send`/`Sync`/`Unpin`; `UnwindSafe` deferred) | Tests. |
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

**Current M4 status:** the enforceable borrow/lifetime surface is implemented
and enforced. `FutureHandle` direct frame borrows are tracked as MIR borrow
views through locals/fields/wrappers/pointer containers and the named extraction
paths (`Option::take`, `Box::into_inner`). Function-type call sites now carry
free (non-HRTB) lifetimes and higher-ranked `FutureHandle` returns; callback
results must have a reference/future source or `'static`, and free-lifetime
capturing callbacks are rejected. Nested aggregate returns resolve the
FutureHandle lifetime through aggregate lifetime arguments; type-erased generic
containers are rejected at safe call boundaries (`SAFE-LIFE-005`); a
non-lifetime wrapper or type alias that erases the nested future lifetime is
covered by `SAFE-LIFE-001`. The remaining async gap is type erasure performed
entirely inside an explicit `unsafe` boundary. Nested struct aggregate
parameters and borrowed nested-field results are accepted, including
multi-lifetime leaf precision. Variant payloads and constant array elements
are path-mapped too, so their borrowed results are accepted; same-lifetime
array elements remain conservatively aliased. Read-only mixed/distinct
multi-borrow calls are accepted. Retaining parameters backed by local or
by-value storage also accept mixed/distinct sources, and a retaining pointee
may contain references with different binders (`&'a mut &'b T`), `'static`, or
a free lifetime carried by a direct reference source, because the call site
checks every source against every stored region. A caller-owned reference slot
is accepted when every source outlives the stored region; its outer borrow
only needs to cover the call. Without that source edge, or for an opaque
retaining type, the call remains an explicit `SAFE-BORROW-099` edge case.

### M5 — Validity/runtime (2 months)

| ID | Feature | Exit |
|---|---|---|
| S5.1 | Validity (init is enforced in M3) | Tests. |
| S5.2 | Remaining validity/FFI checks | Tests. |
| S5.3 | Allocation/layout | Safe-call tests. |
| S5.4 | Bounds enforcement | OOB tests. |
| S5.5 | Overflow/C semantics | Tests. |
| S5.6 | Panic/runtime ABI: abort-only policy | Panic reason/location tests; no unwind profile. |
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

**Current M6 status:** S6.1 (Send/Sync enforcement), S6.3 (threads/scoped
threads), S6.4 (interior mutability) and S6.5 (erased/async Send) are enforced
for the safe surface. S6.2 has the TSAN smoke (`--llvm --thread-tsan`), volatile
unsafe gating, and audited lock/channel primitives. S6.0 is ongoing: the
explicit stdlib impls are audited in `lang/docs/safety/auto-traits-audit.md`;
the audit found and fixed a `&T` Send rule that missed a nested `!Sync`
(`&Cell`/`&RefCell` was accepted as Send). `Rc`/`Weak`, `RefCell`, `Ref`, and
`RefMut` now carry explicit negative impls; public async value types
(`Poll`, `Waker`, `Context`, `SendFutureHandle`) are covered and the erased
`FutureHandle` stays conservatively `!Send`/`!Sync`. Nested conditional
`Unpin` positive/negative impls now use the explicit-impl context. A
conditional impl also applies when its concrete argument is still a generic
parameter whose own declared/where-clause bounds imply the requirement, so a
generic body with `T: Unpin` can use a type whose `Unpin` impl depends on `T`.
Executor/future raw-state structures now carry explicit `!Send`/`!Sync` and
their public raw entry points are unsafe; the iterator `Chunk<T>` remains
conservatively `!Send`/`!Sync` through its raw pointer/length fields. Conditional
positive `Unpin` impls are now materialized for concrete nested bound arguments
(for example `[1]Wrapper<int>`); targets that remain generic parameters stay
conservative. `dyn` without explicit bounds is conservatively `!Send`/`!Sync`
(explicit `+ Send`/`+ Sync` are checked at construction); channel handles are
audited. Thread-pool `Future<T>` is `Send`
for `T: Send`, and `ThreadPool` is `Send + Sync` behind its locked `PoolData`.
Remaining: executor/iterator state-structure coverage (currently conservative
raw-pointer state) and elided/named-lifetime raw-pointer views
(`span`/`mut_span`/`str`/`string_view`). Explicit `'static` views are now
`Send`/`Sync` under the reference rules; an elided view cannot pass a `'static`
or `Send` boundary. `span<T>` requires `T: Sync`; `mut_span<T>` follows
`T: Send`/`T: Sync`; `str`/`string_view` are immutable and unconditional.
The remaining async gap is the type-level `FutureHandle`
lifetime/frame witness in §4.5, not the direct borrow escape.

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
6. **Panic unwinding:** out of scope by policy. Panic always aborts; revisit
   only if a backend makes unwinding practical.
7. **Derive mechanism:** CBI plugin vs built-in; either is acceptable if
   diagnostics are good and compile time stays low.
8. **No-panic effects:** approved proposal 0005. The first `[no_panic]` slice
   is implemented (explicit panic-free callees, runtime panic calls, indirect
   calls, throws); the remaining effects, module default policy, and generic/
   async effect propagation are future work.
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
single-drop change.

*Revision note v4.2 (2026-10-29): S5.6b unwinding is withdrawn by policy.
Panic always aborts; there is no unwinding runtime, `catch_unwind`,
drop-on-unwind, or `UnwindSafe` surface. M6 has started with a safe
non-capturing `std::thread.spawn`/`spawn_void`, `JoinHandle`, `Send + 'static`
enforcement, `SAFE-LIFE-004`, and a TSAN smoke runner; scoped/erased/async `Send` remain.*
*Revision note v4.3 (2026-10-29): S4.2/S1.2 temporary lifetime extension is implemented for variable initializers, including aggregate fields; statement-scoped temporaries now end their storage and are rejected when borrowed past the statement.*
*Revision note v4.4 (2026-10-29): S4.2 higher-ranked function types are implemented: `for<'a>` parsing, binder scope, elided/generic source compatibility, lifetime-only generic function values, and `MIRRegionSignature::hrtb_binders`.*
*Revision note v4.5 (2026-10-29): M4 is complete for the enforceable surface. HRTB call-site region contracts support mutable and borrowed-return callbacks; multi-borrow callbacks are accepted for local referents by pinning their loans to function exit, while parameter/global referents are explicit `SAFE-BORROW-099` rejections. Static lifetime arguments are enforced (`SAFE-LIFE-004`), closing the container/dropck hole for `vector<G<'static>>`. Nested aggregate HRTB and capturing/multi-borrow parameter scoped threads remain.*
*Revision note v4.6 (2026-10-30): M6 starts with join-before-return scoped borrows (`std::thread.scoped_spawn`/`scoped_spawn_mut`), `JoinHandle<T>: Sync` for `T: Send`, and generic-parameter identity fixes. General `Scope` handles, capturing scoped closures, and erased/async `Send` remain for M6.5.*
*Revision note v4.7 (2026-10-30): M6.3 adds a multi-handle shared `scoped_run`/`ScopedScope<T>` (up to 32 workers, join-all-on-drop, unforgeable internal fields). M6.5 remains open: the current `%capture` instance wrapper is not the generated capture layout, so `std::function`/`FutureHandle` still need a concrete typed capture/frame representation before erased/async `Send` can be derived.*
*Revision note v4.8 (2026-10-30): callback capture escape is rejected (`SAFE-LIFE-001`) for direct and aggregate returns, including taint through locals/assignments; `std::default_function_instance` fields are private. By-reference and by-value reference/view captures now require an unsafe construction boundary (`SAFE-UNSAFE-002`); owned by-value captures remain safe. Opaque/erased propagation and `FutureHandle` still need the M6.5 typed representation.*
*Revision note v4.9 (2026-10-30): M6.5 starts with a typed capture-environment witness on capturing function types. Lambdas build a synthetic capture environment at symbol resolution; auto traits and `'static` bounds derive from the captured fields. Plain `std::function` variables acquire the witness from their initializer, `std::typed_function<F,C>` exposes it, and safe `spawn_closure`/`spawn_void_closure` enforce `C: Send + 'static`. Owned captures are accepted; `Rc` and borrowed captures are rejected. `FutureHandle` frame witnesses and capturing scoped closures remain.*
*Revision note v4.10 (2026-10-30): safe async submission starts with `async::spawn_send` and `SAFE-SEND-001`. The compiler walks a direct async callee's frame parameters and locals and requires each to be `Send + 'static`; stored/erased future handles are rejected because the frame cannot be proven. `FutureHandle` itself remains erased/`!Send` until a type-level frame witness is threaded through async lowering.*
*Revision note v4.11 (2026-10-30): capturing scoped closures are safe through `std::thread.scoped_spawn_closure`. The call-site type check permits a borrowed capture witness only at this audited join-before-return boundary; `C: Send` still rejects `Rc` and other non-thread-safe captures. Safe async submission is covered by `async::spawn_send` (`SAFE-SEND-001`) for direct async calls; `FutureHandle` itself remains erased/`!Send`.*
*Revision note v4.12 (2026-10-30): safe async submission now returns a typed `SendFutureHandle<T>` with a conditional `Send`/`Sync` witness, and `block_on_send` drives it. The safe thread/async surface also includes join-before-return capturing scoped closures, and unsafe-created borrowed-callback taint is followed through aggregates, fields, access chains, generic/variant payloads, and erased callback parameters. The TLS/HTTP/server stack was migrated to explicit unsafe bodies at raw-pointer/FFI boundaries; `AwaitNormalizePass` now descends into `unsafe` blocks so awaits inside audited crypto bodies lower as real suspensions. TCC TLS `599/599`, server `10/10`, negative `617`, async `55`. Packed/repr layout and full target-aware C ABI remain explicit unsupported surface (unknown annotations and by-value extern aggregates are rejected, never silently miscompiled); M1/M7 parity and M8 hardening remain.*
*Revision note v4.13 (2026-10-30): the safety audit is whole-tree. `scripts/safety-audit.sh` now compiles every entry point in `lang/docs/safety/audit-modules.txt` with `--check --safety-report json`, merges per-module reports conservatively, and checks a 94-module baseline (`unsafe_blocks 3664`, `unsafe_values 4879`). Webview/universal-test/CBOR raw-pointer and global sites were migrated; cbor tests compile again. `lang/docs/safety/migration-metrics.md` records the methodology and per-domain counts. The safety skill/plan status was reconciled with v4.12. Remaining: M2 C ABI, M5 packed/repr, bare erased callback parameters, heterogeneous/value-passing scoped scopes, M1/M7 parity, and M8 release hardening.*
*Revision note v4.14 (2026-10-30): target C ABI support for by-value structs/unions on x86-64 SysV. The MIR symbol keeps its internal pointer/sret signature and records a separate `c_abi_type`; C prototypes and LLVM declarations use the by-value signature, while calls adapt arguments and returns. Small aggregates are classified per eightbyte (integer/SSE coercion); larger values use `byval`/`sret` memory. `SAFE-FFI-002` now rejects only Chemical variants by value. A linked C fixture exercises small, two-eightbyte, SSE, mixed, memory-class and union values on TCC and LLVM; safety suite `56`. Other target ABIs (Windows x64, ARM) and FFI wrappers/variadics/callbacks remain.*
*Revision note v4.15 (2026-10-30): callback capture storage is always heap-indirect and alignment-aware, removing the inline self-pointer/under-aligned `malloc` hole in `std::default_function_instance`. Packed layout is implemented for structs and unions (`@packed` and `@repr("packed")`; `@repr("C")` is the explicit natural-layout marker), with C `#pragma pack`, packed LLVM aggregate bodies, and packed-layout size/alignment tests. Creating a reference to a packed field is rejected with `SAFE-VALID-008`. `repr("transparent")` remains explicit-unsupported. M6.5 gains safe `std::concurrent.submit_closure` (typed capture, `C: Send + 'static`) and `async::spawn_send_handle` for stored `SendFutureHandle` values; bare erased `std::function` parameters and heterogeneous/value-passing scoped scopes remain.*
*Revision note v4.16 (2026-10-30): a non-Copy by-value callback capture could be returned, assigned, or passed by value out of a reusable lambda; the generated closure call bitwise-copied the capture, so repeated calls could double-own/destroy it. `SAFE-MOVE-002` now rejects the direct move forms (return, var initializer, assignment, call argument). This is a conservative guard, not full Rust-style callable semantics: `FnOnce`/`FnMut`/`Fn` traits and a once-callback type remain M1.16/M6.6 work.*
*Revision note v4.17 (2026-10-30): P1 precision/API slice. MIR move/borrow checking now consumes the checked `impl Copy` marker: a copy-shaped `MoveInit` leaves the source initialized and keeps its loans live (arrays and nested Copy fields included). `@repr("transparent")` is implemented for single-field structs with field-equal size/alignment and C ABI, with `SAFE-REPR-001` rejecting multi-field/inheriting shapes and annotation conflicts. Packed-field reference rejection now propagates through nested fields and array elements. The MIR interpreter now handles `Switch`, `Copy`/`Move`, atomic load/store/compare-exchange/integer RMW, and `AtomicFence`; `Unreachable`/`Throw`, async and inline asm remain explicit hard interpreter errors where they have no comptime meaning. Safety suite `62`, negative `625`.*
*Revision note v4.18 (2026-10-30): HRTB precision. A higher-ranked function-type parameter may now be a single-level aggregate whose binder mentions are direct reference fields; MIR records field regions and maps a field-derived borrowed result back to the caller loan. Multi-borrow HRTB calls now accept parameter/global referents when every non-local referent shares one declared region; local/parameter mixes, distinct non-local regions, and deeper aggregate nesting remain explicit `SAFE-BORROW-099` rejections. Universal/router plugin surfaces were revalidated. Safety suite `63`, negative `631`.*
*Revision note v4.19 (2026-10-31): M6.5 scoped closure breadth. `std::thread.scoped_run_closure` adds a heterogeneous multi-value scope (`ScopedClosureScope`) whose free spawn entries (`scoped_spawn_closure_in`, `_value_in`, `_mut_in`) accept distinct typed capture witnesses and join every worker on scope drop. Value-passing captured closures are available standalone (`scoped_spawn_closure_value`/`_mut`) and through the scope; all are `C: Send` and borrow-confined by the join-before-return boundary. MIR now materializes a nested closure capture through the enclosing closure environment, and `CapturedVariable::known_type` follows the capture chain to the original variable (no accidental `&mut &mut T`). Generic callback parameter types in lambda signatures are deferred to instantiation instead of rejected while still generic, enabling value-passing forms over `T`. The generated-lambda name counters are atomic, so parallel module lowering cannot emit duplicate `__chemda_*` symbols. Remaining M6.5 gaps: bare erased `std::function` parameters and raw `FutureHandle<T>` `!Send`; nested by-value `Copy` aggregate captures are copied (non-`Copy` aggregates remain an explicit lowering error). Main TCC `2366`, LLVM `2364`; negative `641`; safety `63`.*
*Revision note v4.20 (2026-10-31): M6.5 erased-callback guarantee. `std::send_function<T>` is a `%capture<T, default_function_instance, SendStaticCapture>` alias whose marker is only created after verifying the real capture environment is `Send + 'static`; `SAFE-SEND-002` rejects `Rc`, borrowed, and erased witnesses, including an erased `std::function` parameter passed into a `send_function` API. A non-generic API can therefore accept a callback and submit it to `std::thread.spawn_closure` without naming `C`; generic inference now canonicalizes the callback argument type and infers the marker witness. This closes the bare-parameter gap for owned captures. Raw `FutureHandle<T>` remains intentionally `!Send` (a frame can hold non-`Send` values even when `T: Send`); safe submission returns `SendFutureHandle<T>`. Main TCC `2366`, LLVM `2364`; negative `641`; safety `63`.*
*Revision note v4.21 (2026-10-31): M6.5 borrowed-callback parameters. `std::scoped_send_function<T>` uses a `ScopedSendCapture` marker: construction verifies the real capture environment is `Send` (not necessarily `'static`), and the marker is deliberately `!Send`/`!Sync`, so it cannot be handed to an unscoped thread API. New join-before-return entries (`scoped_spawn_closure_erased`/`_value_erased`/`_mut_erased` and the `_in_erased` scope forms) accept it. `@scoped_callback` on a function declares that its `scoped_send_function` parameter is only forwarded to such an API; the call site may then create a borrowed capture without `unsafe`. Without the annotation the call remains `SAFE-UNSAFE-002`. Main TCC `2366`, LLVM `2364`; negative `641`; safety `63`.*

*Revision note v4.22 (2026-10-31): M6.0 concurrency audit. `ArcInner<T>` uses one mutex for both reference counts; independent count locks could double-free when the last strong and last weak owners dropped concurrently, and `ArcWeak::upgrade` could increment the strong count after a concurrent last-strong drop destroyed the value. Count updates, upgrade's test-and-increment, and the last-owner free decision now share that lock; the value destructor runs outside it behind an explicit `value_state` transition so a destructor that touches its own Arc counts cannot deadlock. `ArcWeak<T>` is `Send + Sync` when `T: Send + Sync`. `MutexGuard`, `RwLockReadGuard`, and `RwLockWriteGuard` carry explicit `!Send`/`!Sync` impls. New 200-iteration runtime tests race upgrade/weak-drop against a worker's last-strong drop. `./scripts/test.sh --llvm --thread-tsan` now runs the TSAN smoke test and fails loudly when no TSAN runtime is available. Audit surface dropped to `unsafe_blocks 3661` / `unsafe_values 4890` / `raw_pointer_derefs 969`; main TCC `2366`, LLVM `2364`, @test `54`, negative `641`, safety `63`. Remaining M6 audit targets: remaining std auto-trait coverage and broader TSAN scenarios.*

*Revision note v4.23 (2026-10-31): safety audit. The audit found and fixed four concrete holes. (1) Nested fields bypassed conditional generic `!Send`/`!Sync` impls: `Outer { var w : Wrapper<int> }` was accepted as `Send` even when `impl<T> !Send for Wrapper<T> where T: Copy` applied; `TypeVerifierAutoTraitContext` now evaluates conditional generic positive and negative impls at every nested concrete instantiation. (2) Immutable globals accepted an explicit `impl !Sync` because `SAFE-GLOBAL-002` called structural derivation without the Send/Sync interfaces; it now passes them together with the context. (3) `dyn I` was structurally `Send`/`Sync`/`Unpin` even though the erased implementor could hold `Rc`/raw pointers; a `dyn` backed by `Rc` could be sent through `std::thread.spawn`. Dynamic objects are now conservatively `!Send`/`!Sync`/`!Unpin` until `dyn I + Send/Sync` is represented. (4) Nested conditional `!Unpin` is now treated conservatively as negative. The safety report previously skipped **every** `ImplDefinition` body; it now walks impl bodies, raising the reviewed baseline from `unsafe_blocks 3661`/`unsafe_values 4890` to `3765`/`4898` (`raw_pointer_derefs 1069`, `extern_calls 1296`, `panic_calls 95`) — this is corrected coverage, not new unsafe code. `vector::resize` now checks capacity-doubling overflow; `string::move_data_to_heap` and `async::channel` check allocations; channel `recv_or` futures now hold a receiver count and release it on drop; `Sender`/`Receiver`/`ChannelShared`/`RecvState` fields are internal and channel handles are audited `Send + Sync` for `T: Send`; `ArcWeak` is audited `Send + Sync` under the shared count lock. Known blockers at that audit point: `FutureHandle` lifetime erasure (direct escapes were closed in v4.24), LLVM C ABI not target-gated, `@scoped_callback` trust, `dyn` auto-trait bounds, unfinished async-runtime allocation checks, and local-only audit CI. Tests: TCC main `2366`, LLVM `2364`, @test `54`, negative `647`, safety `64`, async `58`, libs `730`, `MIRTests` `4643`, TSAN pass.*


*Revision note v4.24 (2026-10-31): `FutureHandle` borrow tracking, first slice. MIR now classifies `core::async::FutureHandle` as a borrow view (`MIRBorrowCheck::is_future_handle_type`/`is_borrow_view_type`); async declarations get a synthetic elided return region with outlives edges from every borrowed parameter and by-value aggregate field; ordinary non-`&self` wrappers returning a future carry their reference-parameter loan; by-value future handles and view parameters propagate their input loans through pass-through calls. The borrow checker follows the resulting loan through local assignment, struct fields, returns, and call liveness, rejecting `SAFE-LIFE-001`/`002` for the direct local return, local-scope, free-wrapper, method, struct-field, `Option`, and `string_view` cases. A synthetic region is deliberately not added for every `&self` method because owning futures such as `Channel::recv_or` can outlive their receiver handle; actual body loans still catch borrowing methods. Remaining gap: no type-level `FutureHandle<'a, T>` / caller-visible witness, so unknown/extern callees and generic containers that hide a future behind an unrelated result still need the lifetime model. New negative tests cover direct/local/wrapper/free-wrapper/method/local-view races plus a same-scope positive. Main TCC `2366`, LLVM `2364`, negative `656`, safety `64`, async `58`, libs `730`, `MIRTests` `4643`, TSAN pass.*


*Revision note v4.25 (2026-10-31): P0 safety closure. (1) Every async future/state constructor in `yield.ch`, `time.ch`, `exec.ch`, `blocking.ch`, `combinators.ch`, `reactor.ch`, and the compiler frame hook in `frame.ch` now checks `malloc` before use. (2) `.github/workflows/safety-audit.yml` runs the whole-tree no-growth audit plus negative/safety suites on PRs and `main`. (3) `@scoped_callback` is compiler-checked by `SAFE-SCOPED-001`: the `std::scoped_send_function` parameter may only be forwarded to a known join-before-return sink or another annotated function. (4) `FutureHandle` safe-retaining APIs are rejected by `SAFE-LIFE-005` at the AST call boundary; this covers direct handles, struct/array/`Option` carriers, and unknown/indirect safe callees, while known consumers (`block_on*`, `spawn*`, resolved by exact library namespace) and future-returning APIs remain accepted; a negative test covers a user-defined `concurrent::spawn` spoof. The type-level frame lifetime is tracked in v4.26. (5) LLVM now rejects by-value `@extern` aggregates on non-x86-64-SysV targets with `SAFE-FFI-003`; the C backend still delegates to the host ABI. Main TCC `2366`, LLVM `2364`, negative `664`, safety `64`, async `58`, libs `730`, `MIRTests` `4643`, TSAN pass; audit no growth.*


*Revision note v4.26 (2026-10-31): `FutureHandle<'a, T>` frame lifetime. Async returns carry `'static` for owning frames and an elided lifetime when a parameter/frame value borrows; `spawn_send` requires `FutureHandle<'static, T>` so a borrowed frame cannot cross a safe submission boundary. Return/parameter checks reject a borrowed future returned through the legacy lifetime-less `FutureHandle<T>` spelling. `BaseType::satisfies(Value*)` now preserves a `GenericType` when both sides are generic, so lifetime arguments are not lost before comparison; `GenericType::satisfies` additionally recognizes compiler-generated FutureHandle master/instantiation spellings. Existing positive wrappers/methods were updated to explicit `FutureHandle<'a, T>` signatures. Negative tests `664` (`async_spawn_send_borrowed` added); TCC/LLVM main `2366`/`2364`, async `58`, safety `64`, libs `730`. Remaining: full call-site lifetime inference/unification, references/callback captures, and custom future-consumer wrappers.*


*Revision note v4.27 (2026-10-31): first Fn callable-kind slice. Added `std::Fn<T>`, `std::FnMut<T>`, and `std::FnOnce<T>` aliases with `FnMutCapture`/`FnOnceCapture` markers. `FnMut` calls require a non-`const` local receiver; `FnOnce` calls require a moved receiver, destroy the callback environment after the call, and a second call is rejected by MIR (`SAFE-MOVE-003` plus the destroyed place state). Markers are conservative `!Send`/`!Sync`. Non-`Copy` capture moves remain blocked by `SAFE-MOVE-002`; field-stored `FnOnce` calls are rejected until field drop-flag elaboration. Added four regression tests (`fn_once_single_call`, `fn_once_double_call`, `fn_mut_mutable_call`, `fn_mut_const_receiver`). Negative tests `668`; main TCC/LLVM `2366`/`2364`, async `58`, safety `64`, libs `730`.*


*Revision note v4.28 (2026-10-31): FnOnce capture moves + drop flags. Explicit `std::FnOnce<T>` targets now allow moving a by-value non-`Copy` capture out of the lambda body. Closure capture structs carry one drop flag per destructible by-value capture; the generated destructor skips moved fields, and `lower_moved_captured_variable` transfers the value into a temporary while clearing its flag. Reusable `std::Fn`/`FnMut` targets and second moves remain rejected by `SAFE-MOVE-002`. A runtime test moves a captured `string` out of a `FnOnce` and returns it (`2368` TCC / `2366` LLVM main); negative tests cover reusable-target rejection, double capture move, and the existing call-once cases (`671`). Remaining: full trait bounds/generic call syntax, field-stored `FnOnce`, inferred once lambdas, async closures.*


*Revision note v4.29 (2026-10-31): Fn bounds and generic call syntax. `core::ops::Fn<F>`/`FnMut<F>`/`FnOnce<F>` are now real marker interfaces; generic where-clause checking enforces the callable-kind hierarchy for actual callables. Generic type parameters with `Fn`/`FnMut` bounds can be called through their bound signature, with lambda parameters materialized from that signature. `FnOnce` generic calls remain deliberately deferred because ownership consumption is not modeled at the generic call boundary. A runtime test exercises `call_fn_bound(||() => ...)` on TCC/LLVM. Added negative/positive tests for callable bounds and generic call syntax; negative tests `675`, main TCC/LLVM `2369`/`2367`. Remaining Fn work: field-stored `FnOnce`, inferred once lambdas, `FnOnce` generic-call ownership, broader `FnMut` mutability, async closures.*


*Revision note v4.30 (2026-10-31): Fn field calls and generic call syntax. Field-stored `FnOnce` calls now copy the callback into a temporary, zero the source field, and destroy the temporary; reuse of the zeroed field reaches `__chc_call_moved_fail` instead of dereferencing null. Struct-field lambda initializers mark their `FnOnce` target so capture moves work through the field. `core::ops::Fn`/`FnMut`/`FnOnce` marker bounds and generic `Fn`/`FnMut` call syntax are implemented, with bound-signature materialization for non-capturing generic lambda arguments. Runtime tests cover a field `FnOnce` move and generic bound call. Main TCC/LLVM `2370`/`2368`; negative `675`. Remaining: inferred once lambdas, capturing generic lambda inference, `FnOnce` generic-call ownership, broader `FnMut` mutability, variadic/associated-output trait bounds, async closures.*


*Revision note v4.31 (2026-10-31): FnOnce generic calls + FnMut receiver chains. Generic `FnOnce` bounds now materialize the callback signature for explicit `std::FnOnce` values and non-capturing lambdas; the callback is consumed by its first call and a second call is rejected by MIR (`FnOnce callback cannot be called after it was moved`). `FnMut` receiver checks now walk field chains, so a `const`/immutable holder cannot call a mutable callback. Added runtime tests for generic `FnOnce` consumption and mutable `FnMut` fields, plus negative tests for a second `FnOnce` call and a const `FnMut` field receiver. Main TCC/LLVM `2372`/`2370`; negative `678`. Remaining: inferred once lambdas, capturing generic lambda inference, broader `FnMut` parameter mutability, full variadic/associated-output bounds, async closures.*


*Revision note v4.32 (2026-10-31): inferred and generic capturing Fn callables. Untyped capturing lambda initializers now infer the default function-instance callable type; a body that moves a non-Copy capture upgrades to FnOnce, while non-moving closures remain reusable. Generic Fn/FnMut/FnOnce bounds materialize a capturing expected type from the bound signature, so capturing lambdas infer the concrete callable type (including generic FnOnce capture moves). Added runtime tests for inferred FnOnce, reusable inferred captures, generic capturing Fn calls, and generic FnOnce capture moves; added a negative test for a second inferred FnOnce call. Main TCC/LLVM `2376`/`2374`; negative `679`. Remaining: full variadic/associated-output trait bounds and async closures.*


*Revision note v4.33 (2026-10-31): variadic Fn signatures. MIR function definitions now spell the C variadic tail for both ordinary functions and lambdas, and lambda function types carry `TF_VARIADIC`; generic `Fn`/`FnMut`/`FnOnce` bounds therefore work with variadic signatures and variadic lambda arguments. Associated output is carried by the function type (`Fn<(args...) => Out>`). Added runtime tests for a variadic named callback and a variadic lambda through an `Fn` bound. Main TCC/LLVM `2378`/`2376`; negative `679`. Remaining: async closures.*

*Revision note v4.34 (2026-11-01): async closures. `async |captures|(params) => ...` is now lowered on TCC and LLVM instead of diagnosed. A capturing async closure is conservatively a `FnOnce` callable: the ramp moves the whole `default_function_instance` (including the capture allocation base) into the coroutine frame, `poll` binds captured names through the frame's `fn_data_ptr`, and the frame destroys the instance on normal completion, cancellation, or a never-polled drop. Non-capturing async closures remain reusable function pointers. Explicit `FnOnce`/generic bound targets and field-stored callables are covered; a capturing async closure assigned to `Fn`/`FnMut` is rejected, and a borrowed capture cannot be typed as `FutureHandle<'static, T>` (`SAFE-LIFE-004`). Async suite `68` TCC/LLVM; main `2378`/`2376`; negative `680`; safety `64`; libs `730`; `MIRTests` `4643`.*

*Revision note v4.35 (2026-11-01): `SAFE-LIFE-005` reference and callback-capture closure. `contains_future_handle_ast_type` now recurses through safe references, so `&FutureHandle` (including inside `Option`/arrays/structs) cannot be passed to an unknown safe callee. The argument scan also inspects the real lambda capture list and struct-literal/field initializers, catching a future hidden in a callback whose static `std::function` type erased the capture witness. Added negative tests for a reference escape, a direct capturing lambda argument, a field-stored capturing callback, a callback assigned after declaration, and a future-containing aggregate wrapper returned from a local borrow. Main TCC/LLVM `2378`/`2376`; negative `686`; safety `64`; async `68`; libs `730`; `MIRTests` `4643`. Remaining: full call-site lifetime unification and custom consumer wrappers.*

*Revision note v4.36 (2026-11-01): FutureHandle call-site inference and custom consumers. Reference parameters of future-returning functions are seeded as caller-owned loans (including elided `&T`), so `fn <'a,'b> wrap(x:&'a T) -> FutureHandle<'b,U>` is checked against the region it actually returns (`SAFE-LIFE-003`) instead of synthetic "all inputs outlive the result" edges; those edges now apply only to async functions whose frame retains every input. Aggregate `FutureHandle` returns are detected through references, pointers, arrays, generics, structs, variants, and callback captures. A safe user-defined consumer is accepted when its body only forwards the handle to `block_on`/`spawn*` or another checked consumer; storing, returning, capturing, or passing it to an unknown callee is rejected. Added wrong-region, two-lifetime inference, aggregate-wrapper, custom-consumer, and unsafe-consumer tests. Main TCC/LLVM `2378`/`2376`; negative `691`; safety `64`; async `69`; libs `730`; interpret `1910`; `MIRTests` `4643`.*

*Revision note v4.37 (2026-11-01): field-wise drops + remaining FutureHandle inference. Local and by-value-parameter aggregates with generated drop glue now arm per-field/per-element drop flags on whole construction and clear the moved field's flag on a move, including nested fields and whole-array fields. Moves from user-destructor aggregates, async frames, globals, temporaries/expressions, and references/pointers are rejected with `SAFE-MOVE-004` rather than double-dropping. FutureHandle aggregate field loans are lifted through by-value wrappers, `Box`/`Rc`/`Arc` pointer containers and `Option::take`/`Box::into_inner` extractions carry the source loan, custom consumers accept local aliases, and owning `FutureHandle<'static, T>` captures are no longer rejected. Added runtime move/drop tests and negative unsound-acceptance tests. Main TCC/LLVM `2378`/`2376`; negative `702`; safety `70`; async `69`; libs `730`; interpret `1910`; `MIRTests` `4643`.*

*Revision note v4.38 (2026-11-01): target-aware C ABI. `MIRABI` now classifies by-value `@extern` structs/unions per target: x86-64 System V (eightbyte INTEGER/SSE, memory class), Windows x64 (1/2/4/8-byte integer aggregates, memory otherwise), and AArch64 AAPCS64 (HFAs of 1–4 floats/doubles including unions/arrays, other <=16-byte composites in integer registers, memory otherwise). Register parts carry their exact aggregate offset, so HFA loads/stores no longer assume 8-byte spacing. LLVM adds `byval` only on System V memory parameters; Windows/AArch64 use plain pointers, matching Clang. `MIRABIKind` is recorded in `MIRModuleLayout` from the target triple, and unsupported targets (i386, ARM32, Windows ARM64, WASM, unknown) still reject by-value extern aggregates with `SAFE-FFI-003`. Added `MIRTests` classifier coverage and an LLVM-target negative test. Main TCC/LLVM `2378`/`2376`; negative `703`; safety `70`; async `69`; libs `730`; interpret `1910`; `MIRTests` `4682`.

*Revision note v4.39 (2026-11-01): FutureHandle higher-order inference. Function-type region signatures now collect free (non-HRTB) lifetime names from reference parameters, generic lifetime arguments such as `FutureHandle<'a, T>`, arrays and aggregate fields; function values of lifetime-generic declarations are marked higher-ranked. Indirect calls with a free-lifetime contract use the signature for result-loan mapping but keep the conservative escape mark, while fully higher-ranked contracts retain the existing non-retaining assumption. `MIRBorrowCheck` now recovers the sret destination for capturing lambdas (the sret parameter follows the hidden capture-environment pointer), so returning a future that borrows a lambda capture is checked. Callbacks returning a free `FutureHandle` require a reference/future parameter source or `'static`; HRTB `FutureHandle` returns are accepted; capture-owning lambdas returning a free/legacy future are rejected at the call site because the erased capture may die first. Added positive/negative coverage for free and HRTB callback returns and opaque capture-return cases. Main TCC/LLVM `2378`/`2376`; negative `711`; safety `70`; async `69`; libs `730`; interpret `1910`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.40 (2026-11-01): FutureHandle nested aggregates and opaque containers. Function-type region signatures now recover the single FutureHandle lifetime from nested return types (`Outer<'a> -> Inner<'a> -> FutureHandle<'a>`) and prefer the aggregate's lifetime argument over a nested declaration's erased binder. HRTB aggregate returns containing a FutureHandle are accepted when the result lifetime has a reference/future source, and the result loan propagates through the nested return. A safe call returning a type-erased generic container whose representation does not expose the future in a field is rejected with `SAFE-LIFE-005`; inspectable lifetime-carrying wrappers remain accepted. MIRTypeRecord carries a conservative `contains_future_handle` fact for erased generic containers. Added nested-aggregate positive/local-borrow-negative tests and an opaque-container negative test. Main TCC/LLVM `2378`/`2376`; negative `714`; safety `70`; async `69`; libs `730`; interpret `1910`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.41 (2026-11-01): `dyn` fat-pointer first slice. `dyn I` now reports target-aware two-pointer size and pointer alignment (instead of hard-coded 64-bit values). Dynamic dispatch peels one `&`/`*` indirection, so `Box<dyn I>::get()` (`&dyn I`) dispatches through the fat pointer's vtable rather than falling back to a static interface method; the same path works on TCC and LLVM. `Box<dyn I>` is documented as a boxed fat-pointer **view** (the generic `Box<T>` owns the two-word record, not the referent). Safe construction is conservatively rejected with `SAFE-DYN-001` until boxed-view borrow tracking lands; owning erased values need a separate drop contract. Added `sizeof`/`alignof` tests, an `&dyn` dispatch test, a boxed-view runtime test under `unsafe`, and a negative `SAFE-DYN-001` test. Design: `lang/docs/dyn-object-model.md`. Main TCC/LLVM `2381`/`2379`; negative `715`; safety `70`; async `69`; libs `730`; interpret `1913`; `MIRTests` `4682`; audit no growth (intentional `raw_pointer_types` +2 from the boxed-dyn test).

*Revision note v4.42 (2026-11-01): `dyn` borrow tracking and owning `DynBox`. MIR type records now carry a conservative `contains_borrow_view` fact for `dyn` and aggregates that store a `dyn` fat pointer. A fat pointer's data-word field store lifts the referent loan to the view; the vtable word is ignored. `Box<dyn I>` is therefore borrow-checked as a view: returning a boxed local is `SAFE-LIFE-001`, and a boxed view that outlives its referent scope is `SAFE-LIFE-002`; safe construction no longer needs `unsafe` or `SAFE-DYN-001` (retired). View-result loan mapping is narrowed to `box`, so copy-owning APIs such as `get_owned_parameter(view)` do not inherit the argument's loan. Added `std::DynBox<I>` and `std::dyn_box<T, I>`: the constructor allocates the concrete value once, builds a `dyn I` against the heap address, and moves the owning `Box<T>` into an empty capturing closure, which drops it exactly once. `DynBox` is excluded from the borrowed-view classification because it owns the heap storage; destructor tests cover drop-once. Existing dynamic tests were corrected to keep a referent alive instead of returning a view of a by-value parameter. Main TCC/LLVM `2381`/`2379`; `@test` `56`; negative `716`; safety `70`; async `69`; libs `730`; interpret `1913`; `MIRTests` `4682`; audit no growth (reviewed `raw_pointer_derefs 1068`, `raw_pointer_types 9361`); TSAN pass.

*Revision note v4.43 (2026-11-01): `DynBox` stored drop contract. `DynBox<I>` now stores `{ fat, data, drop }` instead of an owner closure: `dyn_box<T, I>` allocates `T` once, captures the heap pointer, stores the monomorphized `drop_erased<T>`, and forgets the backing `Box<T>`. The destructor calls the stored function once, which runs the normal `delete` path (destructor + free). This removes the owner-closure allocation while preserving drop-once semantics and the safe `DynBox::get`/`get_mut` surface. `DynBox` remains move-only and excluded from borrowed-view tracking because it owns the heap storage. Main TCC/LLVM `2381`/`2379`; `@test` `56`; negative `716`; safety `70`; async `69`; libs `730`; interpret `1913`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.44 (2026-11-01): `dyn` upcasting. `DynamicType::satisfies` now accepts `dyn Derived` → `dyn Base` when Base is reachable through Derived's **first** inherited interface. The vtable builder flattens that chain first, so Base's method slots are a prefix of Derived's vtable and the fat pointer can be copied without thunking or adjustment. Upcasting through a later/incompatible inheritance branch and downcasting remain rejected. Added a runtime test covering `dyn Derived` assignment to `dyn Base` and calls through both vtables on TCC/LLVM/interpretation. Main TCC/LLVM `2382`/`2380`; `@test` `56`; negative `716`; safety `70`; async `69`; libs `730`; interpret `1914`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.45 (2026-11-01): `dyn + Send/Sync`. `DynamicType` now carries explicit Send/Sync auto-trait bits; the parser accepts `dyn I + Send + Sync` in type position and `dyn<I + Send + Sync>(value)` in construction. `DynamicType::satisfies` requires the target's bounds to be a subset of the source's, so `dyn I` cannot silently become `dyn I + Send`. Auto-trait derivation consults the bits instead of treating every dynamic object as unbound; `dyn` without a bound remains `!Send`/`!Sync`. Type verification checks the concrete value's structural/impl Send/Sync at construction and reports `SAFE-SEND-003` on failure. Added positive Send+Sync construction/where-clause coverage and a negative `Rc`-backed construction test. `+ Unpin` remains unimplemented. Main TCC/LLVM `2383`/`2381`; `@test` `56`; negative `717`; safety `70`; async `69`; libs `730`; interpret `1915`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.46 (2026-11-01): `dyn + Unpin`. `DynamicType` now represents an explicit `+ Unpin` bit; `core::Unpin` is cached in `CoreNodes` (it lives inside `namespace core`, unlike top-level Send/Sync), and `type_satisfies_unpin` consults the bit instead of rejecting every dynamic object. Construction `dyn<I + Unpin>(value)` checks the concrete value through the same structural/negative-impl path used by `where T: Unpin`; `PhantomPinned` is rejected with `SAFE-SEND-003`. Added positive `+ Unpin` coverage and a negative `PhantomPinned` construction test. Main TCC/LLVM `2383`/`2381`; `@test` `56`; negative `718`; safety `70`; async `69`; libs `730`; interpret `1915`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.47 (2026-11-01): `dyn` object-safety checks. `is_object_safe` now requires every interface method to have a self receiver (static/zero-receiver methods have no vtable slot) and rejects `Self` appearing directly or nested in a parameter/return type, including pointers, arrays, generics and function types. The self parameter is excluded from the `Self` scan. Added a negative static-method object-safety test. Main TCC/LLVM `2383`/`2381`; `@test` `56`; negative `719`; safety `70`; async `69`; libs `730`; interpret `1915`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.48 (2026-11-01): tuples (first slice). Tuple types `(T1, T2)` and literals `(e1, e2)` are represented as structural inline `StructType`s with generated `_0`, `_1`, ... members; `._0` access parses numeric field access and nested tuples work. MIR interns tuple aggregates by element type ids with a structural C tag, so prototypes and bodies agree. Tuple facts include structural equality, element-wise `Copy`/`Send`/`Sync`/`Unpin`, parameter/return passing, and field-wise drop-once (including destructor-bearing elements). The parser speculatively tries the lambda form for `(a, b) => ...`, rolls back on failure, and suppresses speculative diagnostics; multi-line tuple types/values and function-type parameter lists are supported. Added six positive tuple tests (including a `Copy` bound test) and a tuple element type mismatch negative. CBI plugin/universal/router suites recompiled and pass. Main TCC/LLVM `2389`/`2387`; `@test` `56`; negative `720`; safety `70`; async `69`; libs `730`; interpret `1921`; plugins `1305`; `MIRTests` `4682`; audit no growth after a reviewed `raw_pointer_derefs 1068 -> 1069` baseline bump for the tuple drop test; TSAN pass. Design/limitations: `lang/docs/tuples.md`; tuple structs, destructuring patterns, and a real unit tuple value remain.

*Revision note v4.49 (2026-11-01): `&[T]` and `&str` view slice. `&[T]` is parsed as `std::span<T>`; array and `&vector<T>` constructors provide the view, the new `Index` impl enables `view[i]`, and existing MIR view tracking rejects returning a view of a local (`SAFE-LIFE-001`). `&str` is parsed as the new `std::str`, a non-owning `std::string_view` wrapper whose safe constructors validate UTF-8 and abort on invalid bytes; `str`/`string_view` views carry source loans and reject local escapes. Added positive array/vector/string/literal tests, UTF-8 valid/invalid validation coverage, and negative `&[T]`/`&str` local-return tests. The audit baseline was intentionally updated for `std::str` (`panic_calls 104 -> 106`, `raw_pointer_types 9361 -> 9363`, `unsafe_blocks 3774 -> 3775`, `unsafe_values 4901 -> 4902`). Main TCC/LLVM `2397`/`2395`; `@test` `56`; negative `722`; safety `70`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass. Design/limits: `lang/docs/slices.md`; bare `[T]`, `Box<[T]>`, mutable slices, and explicit `&'a str` lifetimes remain.

*Revision note v4.50 (2026-11-02): bare `[T]`, mutable `&mut [T]`, and owning `Box<[T]>` slices. `[T]`/`&[T]` map to `std::span<T>`; `&mut [T]` maps to `std::mut_span<T>` (constructed from `&mut vector<T>`, with writes and `IndexMut`); `Box<[T]>` maps to the vector-backed owning `std::boxed_slice<T>`. The `IndexMut` interface now declares `index_mut` so shared `Index` and mutable `IndexMut` can coexist without duplicate C members. Shared-slice assignment is rejected; mutable views of arrays require the explicit unsafe pointer/length constructor. Added bare/vector/owning/mutable tests and negative tests for shared mutation, array-to-mutable-view, and mutable-view escape. Audit baseline was updated for the new std view types (`raw_pointer_types 9363 -> 9367`); no unsafe/panic growth. Main TCC/LLVM `2401`/`2399`; negative `725`; safety `70`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass. Design/limits: `lang/docs/slices.md`; explicit `&'a` lifetimes, array-backed mutable slices, and sub-slicing remain.

*Revision note v4.51 (2026-11-02): safe mutable array slices and subviews. `&mut [T]` now constructs from `&mut arr` as well as `&mut vector<T>`; const arrays and plain array values are rejected, and mutable array views are borrow-checked for local escapes. Added `subview(start,end)` for `span`, `mut_span`, and `Box<[T]>` (`subview_mut` for the boxed form), plus bounds-panic coverage. Range syntax, `split_at`, `split`, and `windows`/`chunks` remain future work. Audit baseline updated for the added std methods (`panic_calls 106 -> 107`, `raw_pointer_types 9367 -> 9368`, `unsafe_values 4902 -> 4904`). Main TCC/LLVM `2406`/`2404`; negative `727`; safety `71`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.52 (2026-11-02): `split_at` and field-index MIR fix. `span`, `mut_span`, and `Box<[T]>` expose `split_at`; shared splits return `slice_pair<T>`, mutable splits return `mut_slice_pair<T>`, with checked bounds and panic coverage. MIR indexing now recognizes struct-like receivers reached through field access (e.g. `parts.first[i]`), not only direct places, so slice fields index through `Index`/`IndexMut` correctly. `split`, range syntax, `windows`/`chunks`, and patterns remain. Audit baseline updated (`panic_calls 107 -> 108`, `raw_pointer_types 9368 -> 9370`, `unsafe_values 4904 -> 4907`). Main TCC/LLVM `2409`/`2407`; negative `727`; safety `72`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.53 (2026-11-02): range indexing, windows/chunks, and checked UTF-8. `view[a..b]`, `view[..b]`, `view[a..]`, and `view[..]` parse into `subview` calls (shared, mutable, boxed, and string views). `windows(width)`/`chunks(width)` expose non-allocating `span_windows<T>`/`span_chunks<T>` cursors with `.get(i)`; zero chunk width and invalid ranges panic. `std::try_from_utf8(view)` returns `Result<str, string_view>` and rejects invalid bytes without aborting. Lexer `..` no longer consumes the first dot as a decimal point; CBI token enums were synchronized. Generic slice `split`, slice patterns, and lifetimes remain. Audit baseline updated (`panic_calls 108 -> 111`, `unsafe_values 4907 -> 4909`; raw pointer total 9370). Main TCC/LLVM `2416`/`2414`; negative `727`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.54 (2026-11-02): predicate slice splitting. Added `split_by(predicate)` for `span`, `mut_span`, and `Box<[T]>`, returning a non-allocating `span_split<T>` cursor with checked `.get(i)`. Generic delimiter `split`/`split_by` remains blocked on generic `PartialEq` operator resolution; `split_by` with a predicate is available, and `&str`/`string_view` keep `.split(delim)`. Audit baseline updated (`panic_calls 111 -> 112`, `unsafe_values 4909 -> 4912`, `raw_pointer_derefs 1069 -> 1071`). Main TCC/LLVM `2417`/`2415`; negative `727`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.55 (2026-11-02): generic delimiter slicing + instantiation gating. Generic equality/comparison now defers the operand check when either side is an unresolved generic parameter and re-checks it at concrete instantiation, so user generic functions and generic-struct methods can use `==` without weakening concrete type checking. Monomorphized member bodies whose `PartialEq`/`Eq` where-bound is not satisfied by the concrete struct argument are no longer lowered (the body is cleared), avoiding errors from unused helper methods such as `span<ImportSymbol>.split`; real calls still fail where-clause verification. `span`, `mut_span`, and `Box<[T]>` now expose `split(delim)` returning `span_delim_split<T>`, and `split_by(predicate)` remains available. Audit baseline updated (`panic_calls 112 -> 113`, `unsafe_values 4912 -> 4915`, `raw_pointer_derefs 1071 -> 1073`). Main TCC/LLVM `2418`/`2416`; negative `728`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.56 (2026-11-02): explicit slice/string lifetimes. `&'a [T]`, `&'a mut [T]`, and `&'a str` now preserve the lifetime argument on the `std::span`/`std::mut_span`/`std::str` alias instead of discarding it during parsing. MIR region signatures read the declared lifetime, so wrong return regions fail with `SAFE-LIFE-003` and shorter-lived slices/strings cannot satisfy `&'static` (`SAFE-LIFE-004`; the static-mismatch helper now keeps a written `GenericType` instead of canonicalizing away its lifetime args; a missing source lifetime argument is treated as an elided borrow, not `'static`, closing the elided `&[T]`/`&str` static-escape path). Elided forms retain ordinary inference. Added shared/str identity and `where 'a : 'b` positive coverage plus wrong-region and static-lifetime negative tests. Main TCC/LLVM `2419`/`2417`; negative `729`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.57 (2026-11-02): unsafe-declaration audit metrics. The whole-tree safety report now counts `unsafe func` declarations (`unsafe_func_decls`) and `unsafe impl` declarations (`unsafe_impl_decls`) separately, closing the gap where these boundary witnesses were invisible in the audit. Reviewed baseline now records `unsafe_func_decls 36` and `unsafe_impl_decls 15`; all other reviewed totals are unchanged (`panic_calls 113`, `unsafe_values 4915`, `raw_pointer_derefs 1073`, `raw_pointer_types 9370` manually pinned). Migration metrics were refreshed from the baseline. Main TCC/LLVM `2419`/`2417`; negative `729`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.58 (2026-11-02): FutureHandle erasure coverage. Added negative tests for a `type Alias = FutureHandle<int>` return and a non-lifetime `Holder<FutureHandle<int>>` wrapper; both cannot return a future that borrows function-local storage (`SAFE-LIFE-001`). The documented remaining FutureHandle gap is now only user type erasure performed entirely inside an explicit `unsafe` boundary. Main TCC/LLVM `2419`/`2417`; negative `730`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.59 (2026-11-02): multi-borrow HRTB precision. Function-type region signatures now record whether each parameter exposes storage that can retain a reference (`param_may_retain`). A multi-borrow higher-ranked callback with two or more reference inputs is accepted with mixed local/parameter/global referents and distinct caller regions when no parameter can retain a borrow; all loans are pinned to function exit. A mutable reference slot, reference-containing pointee, generic parameter, or callback parameter still uses the conservative equal-region/local-only rule and rejects with `SAFE-BORROW-099` (negative coverage via a generic `&mut T` slot). Tests that previously expected rejection for read-only mixed/distinct regions are now positive; deeper nested aggregate HRTB remains an explicit rejection. Main TCC/LLVM `2419`/`2417`; negative `731`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.60 (2026-11-02): nested aggregate HRTB parameters. `hrtb_aggregate_param_ok` now recursively validates nested struct aggregates of direct reference fields and rejects only reference-capable nested storage. A callback may take `Outer<'b> { view: View<'b> }` by value and read through it; a real nested-literal call was added as positive coverage. A borrowed result taken from a nested field is still rejected because MIR lacks path-wise leaf-field mapping (`SAFE-BORROW-099`); direct field results remain supported. Main TCC/LLVM `2419`/`2417`; negative `732`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.61 (2026-11-02): nested aggregate HRTB results. MIR now records whole-aggregate field-source places, follows the nested field-source chain to collect leaf reference loans, and binds them to the aggregate parameter's declared region. A callback returning `&'b int` from `Outer<'b> { view: View<'b> }` now keeps the source loan live; an alias test confirms a later mutable borrow is rejected with `SAFE-BORROW-001`. Type-verifier nested lifetime mapping now propagates the parent aggregate's actual lifetime into nested declarations. Remaining HRTB edge cases are variant/array nested payloads, multi-lifetime leaf precision beyond the aggregate's first lifetime argument, and the conservative retaining multi-borrow rule. Main TCC/LLVM `2419`/`2417`; negative `733`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.

*Revision note v4.62 (2026-11-02): HRTB edge cases. Nested field records now carry a full root-to-leaf constant path and each leaf resolves its own declared lifetime, so `Pair<'a,'b>` results borrow only the selected leaf (a sibling remains mutable). Variant and array nested payload parameters are validated recursively and accepted for read-only callbacks; borrowed variant/array results remain explicit `SAFE-BORROW-099` rejections pending element/payload loan mapping. MIRTests was updated for the path field. Main TCC/LLVM `2419`/`2417`; negative `736`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.
*Revision note v4.63 (2026-11-02): variant and array HRTB results. `MIRParamFieldRegion` paths now carry typed field/index steps. The function-type signature builder records variant payload leaves (`<Member>.<param>`) and constant array element leaves, and `MIRBorrowCheck` tracks fixed-array element source places and loans, carrying them through a whole-array local move. A callback returning a reference from a variant payload (`Maybe<'a>.Some(View<'a>)`) or a constant array element (`Refs<'a>.items[0]`) now keeps the source loan live; alias tests reject a later mutable borrow with `SAFE-BORROW-001`. Same-lifetime array elements are conservatively aliased because a region does not carry an index; fixed arrays larger than 64 elements (and unsized arrays) keep the explicit `SAFE-BORROW-099` rejection. Type verification now substitutes a generic variant/struct payload's type parameter from the concrete type argument and uses the master body for generic containers, so `for<'b> (m: Option<View<'b>>) => &'b int` and `Wrap<View<'b>>` are accepted and path-mapped. Main TCC/LLVM `2419`/`2417`; negative `739`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.
*Revision note v4.64 (2026-11-02): retaining HRTB storage precision. `MIRBorrowCheck` now classifies each retaining parameter's loan-target storage. A local/temporary or by-value parameter copy dies with the call, so it may retain an argument borrow even when the sources are mixed local/parameter/global and have distinct caller regions; every loan is still pinned to function exit. Retaining storage reached through a reference parameter is caller-owned, so the equal-region rule remains and a mixed local/parameter source is rejected. Added `hrtb_retaining_local_slot_mixed_ok`; the existing generic nonlocal-slot negative stays. Main TCC/LLVM `2419`/`2417`; negative `740`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.
*Revision note v4.65 (2026-11-02): same-binder retaining pointees. Type verification now allows a higher-ranked reference pointee whose nested references all use the same binder (`&'a mut &'a T`, including inside a by-value aggregate), while a pointee using a different binder or a free/`'static` lifetime remains `SAFE-BORROW-099`. Combined with the v4.64 storage-location rule, a local or by-value slot can retain a source borrow; a caller-owned reference slot still requires one shared region. Added positive `hrtb_retaining_aggregate_local_slot_ok` and negatives for a caller-owned slot with distinct regions and for a mismatched-binder pointee. Main TCC/LLVM `2419`/`2417`; negative `742`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.
*Revision note v4.66 (2026-11-02): caller-owned retaining HRTB slots. Region signatures now record the named regions a parameter's retaining storage can hold (`param_retain_regions`); for `slot: &'o mut &'i T` that is `'i` while `param_regions` remains the outer `'o`. At a call, `MIRBorrowCheck` resolves each loan to the caller's declared region and accepts a caller-owned retaining slot when the caller's `where` outlives graph proves the slot's outer borrow and every source outlive the stored region. Without the proof the call still rejects, so the rule remains a proof requirement rather than a guessed acceptance. Added `hrtb_retaining_nonlocal_slot_outlives_ok`; the no-bound distinct-region negative stays. Main TCC/LLVM `2419`/`2417`; negative `743`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1921`; `MIRTests` `4682`; audit no growth; TSAN pass.
*Revision note v4.67 (2026-11-02): tuple slice completion. Flat tuple destructuring (`var (a, b) = value`) now materializes the source into an owned snapshot, binds each element to a local, moves destructible fields with their drop flags cleared, respects `const`, and allocates async-frame slots when bindings cross an `await`. Named tuple structs (`struct Point(int, int)`) are normal `StructDefinition`s with `_0...` fields and a generated positional constructor; generic tuple structs instantiate through the normal generic path. The unit tuple `()` is a real zero-element structural tuple rather than `void`; an omitted return type still means `void`. Tuple patterns work for structural tuples and named tuple structs. Remaining: nested/wildcard patterns, tuple patterns in `if`, and tuple methods/inherent impls. Main TCC/LLVM `2427`/`2425`; negative `747`; safety `74`; async `69`; libs `730`; plugins `1305`; interpret `1929`; `MIRTests` `4682`; audit no growth after a reviewed `raw_pointer_types 9370 -> 9371` bump for the generated destructor `*mut self` of the destructible tuple-struct test.
*Revision note v4.68 (2026-11-02): array and exact slice patterns. `var [a, b] = arr`, exact-length patterns, one `..` at any position, and `_` discards now bind fixed-size array elements into owned locals. Patterns are evaluated once and copy element values before later source mutation; Copy aggregate elements are supported. Non-Copy array destructuring rejects with `SAFE-MOVE-004` until per-index moves are modeled. `if(var [a, b] = view)` supports exact refutable patterns on `[T]`/`&[T]`: it checks the runtime `_size` and binds through the view. Slice rest patterns, `&mut [T]`/`Box<[T]>` patterns, and statement forms with an `else` path remain explicit rejections. Added positive exact/rest/wildcard, Copy-aggregate, and slice-condition tests plus arity, non-Copy, and slice-rest negatives. Main TCC/LLVM `2429`/`2427`; negative `750`; safety `74`; async `69`; TCC libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4682`; audit no growth.
*Revision note v4.69 (2026-11-02): associated-type declarations and impl bindings. An interface may declare `type Item` (including on a generic interface) and every implementation must provide a matching `type Item = T`; missing bindings are a compile error. Associated declarations are not emitted as C typedefs. Projections (`Self::Item`/`T::Item`), associated bounds/equality constraints, and GAT parameters remain open and are the next step toward S1.12. Design/limits: `lang/docs/associated-types.md`. Main TCC/LLVM `2429`/`2427`; negative `752`; safety `74`; async `69`; TCC libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4682`; audit no growth.
*Revision note v4.70 (2026-11-02): associated-type projection slice. `Self::Item` now parses as a qualified type projection and resolves through the enclosing interface's associated declaration. Return-position projections are accepted for static dispatch through an implementation; parameter-position projections are rejected because the interface prototype would emit a void placeholder. Container-scoped type aliases are indexed as children so the projection chain can resolve. Interfaces with associated types are marked non-object-safe until projections carry a real vtable type. `T::Item`, bounds/equality constraints, dyn projections, and GAT parameters remain. Main TCC/LLVM `2429`/`2427`; negative `754`; safety `74`; async `69`; TCC libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4682`; audit no growth.
*Revision note v4.71 (2026-11-02): stdlib auto-trait audit. Found and fixed a soundness bug in the shared-reference rule: `T: Send` recursively checked `&U` with the Send interface where the Sync interface was required, so an explicit `!Sync` was missed and `&Cell`/`&RefCell` was accepted as Send. Both the direct where-clause path and generic-impl bound path now pass the real Send/Sync interfaces. Added explicit `!Send`/`!Sync` for `Rc`/`Weak`, `RefCell`, `Ref`, and `RefMut`; the remaining explicit impls were reviewed against their invariants. New regression tests cover `&NotSync`, `&RefCell`, the generic-impl path, and the `&mut T: Send` positive case. Design/limits: `lang/docs/safety/auto-traits-audit.md`. Main TCC/LLVM `2429`/`2427`; negative `760`; safety `74`; async `69`; TCC libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4682`; TSAN pass; audit no growth.
*Revision note v4.72 (2026-11-02): S6.0 async/Unpin completion. Nested conditional `Unpin` now goes through the explicit auto-trait context: a conditional positive `impl<T> Unpin for W<T> where T: Unpin` is evaluated at a concrete nested instantiation, while conditional `!Unpin` still wins and the generic-impl bound path stays conservative. Added public async value auto-trait coverage (`Poll<T>`, `Waker`, `Context`, `SendFutureHandle<T>`, with erased `FutureHandle` conservatively `!Send`/`!Sync`); executor/future state structures and `Chunk<T>` remain conservative raw-pointer state. Design/limits: `lang/docs/safety/auto-traits-audit.md`. Main TCC/LLVM `2429`/`2427`; negative `762`; safety `74`; async `70`; TCC libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4682`; TSAN pass; audit no growth.
*Revision note v4.73 (2026-11-02): legacy lock guards. `lock_guard` and `recursive_lock_guard` now carry explicit `!Send`/`!Sync` impls, matching `MutexGuard`/`RwLock` guards; both hold a raw mutex pointer and unlock in their destructor. Negative tests use type-only Send assertions. Negative suite `764`. Audit doc updated.
*Revision note v4.74 (2026-11-02): stdlib residual auto-trait conservatism. Thread-pool `Future<T>` now has `Send` for `T: Send` (the promise is mutex/condvar protected and `get` is the only `T` transfer), and `ThreadPool` has audited `Send + Sync` (private heap `PoolData`, locked safe submission, `Send + 'static` capture witness). Added type-only positive/negative coverage and a thread-pool test that consumes `Future<int>` on another thread. The pointer-backed `span`/`mut_span`/`str` views and executor/iterator raw state remain conservative because their types do not carry a lifetime/ownership witness; the generic-impl `Unpin` bound path remains conservative. Negative suite `765`; main TCC/LLVM `2430`/`2428`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4682`; TSAN pass; audit no growth.

*Revision note v4.75 (2026-11-02): C ABI classifier breadth (P0). `MIRABI` classification is now parameter/return-aware, which is required because i386 Windows and ARM32 return small aggregates in registers while passing parameters under different rules. Added i386 System V (all aggregate parameters byval, all aggregate returns sret), Windows i386 (byval parameters; i8/i16/i32/i64 returns for 1/2/4/8-byte aggregates, sret otherwise), and ARM32 AAPCS soft-float/hard-float (HFA register parts, 4/8-byte integer word splitting up to Clang's 64-byte threshold, small-register returns, larger non-HFA sret). Target selection records `hard_float` from the `*eabihf` environment in both LLVM and non-LLVM target parsers; the build-script `TargetData` mirror is extended at the end to preserve CBI field layout. LLVM applies `byval` for the new memory-class targets, and `MIRTests` covers every new classifier plus positive target-gate checks for i386 and ARM32. Windows ARM64 remains fail-closed with `SAFE-FFI-003`. By-value aggregates passed through a variadic `@extern` call are rejected with `SAFE-FFI-004` instead of handing the callee an internal aggregate pointer; scalar varargs remain supported. Remaining from the P0 item: Windows ARM64, variadic scalar default-promotion coverage, callback calling-convention auditing, and full FFI wrapper auditing. Main TCC/LLVM `2430`/`2428`; negative `767`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4726`; TSAN pass; audit no growth.

*Revision note v4.76 (2026-11-03): extern callback ABI audit. `TypeVerifier` now receives the job's `TargetData`, and `VisitFunctionDecl` audits every `@extern` signature for callback ABI mismatches. `SAFE-FFI-005` rejects callbacks with a by-value struct/union parameter or return (the internal callback ABI would pass a pointer/sret where C expects the aggregate), capturing functions used as C callbacks, and every callback on 32-bit Windows until stdcall/cdecl callback metadata is represented. Pointer-to-aggregate callback parameters remain accepted, and scalar/pointer callbacks continue to compile on the supported 64-bit conventions. Added aggregate-parameter, aggregate-return, scalar-positive, pointer-to-aggregate-positive, and i386-Windows negative coverage. Remaining from the P0 item: Windows ARM64, variadic scalar default-promotion coverage, explicit stdcall callback types, and full FFI wrapper auditing. Main TCC/LLVM `2430`/`2428`; negative `772`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4726`; TSAN pass; audit no growth.

*Revision note v4.77 (2026-11-03): Microsoft ARM64 C ABI. Added `MIRABIKind::WindowsARM64` and a classifier matching Clang: HFAs use FP parts; non-HFA composites up to 8 bytes travel in x0, 9-16 bytes in x0/x1, and larger composites use caller-owned memory. Returns use the exact integer width for <=8 bytes, x0/x1 for 9-16 bytes, and sret above 16. LLVM uses plain pointers (no `byval`) for Windows ARM64 memory parameters, as Clang does. Target selection now maps `aarch64-*-windows-*` to the new kind; `SAFE-FFI-003` remains fail-closed for RISC-V/WASM/unknown targets. MIRTests covers parameter/return, HFA, 12/16-byte split, and memory-class cases; the negative target gate now uses RISC-V as the unsupported target and adds a positive Windows ARM64 target check. Remaining from the P0 item: variadic scalar default-promotion coverage, explicit stdcall callback types, and full FFI wrapper auditing. Main TCC/LLVM `2430`/`2428`; negative `772`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.78 (2026-11-03): variadic scalar promotions audited. The existing LLVM call lowering already applies C default argument promotions (float -> double, bool/i8/u8/i16/u16 -> i32 with source signedness), but this slice pins the behavior with a linked C fixture. The C function reads `double`, `int`, `int`, `int` through `va_arg`, and the Chemical caller passes `2.5f`, `(-7) as i8`, `true`, and `200 as u8`; both TCC (host C promotions) and LLVM (MIR `coerce_args`) return the expected encoding. Added `lang/tests/src/basic/varargs_promotions.ch` and the C fixture. Remaining from the C ABI P0 item: explicit stdcall callback types and full FFI wrapper auditing. Main TCC/LLVM `2431`/`2429`; negative `772`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.79 (2026-11-03): 32-bit stdcall fail-closed. `MIRSymbolRecord` now carries the source `@stdcall` fact, populated by the declaration pass and both function-interning paths. The LLVM emitter rejects any 32-bit `@stdcall` symbol with `SAFE-FFI-006` instead of silently emitting a cdecl declaration/call; the C backend keeps its `__chem_stdcall` spelling. Added an i386-Windows negative test. Remaining from the C ABI P0 item: actual 32-bit stdcall support/callback metadata and full FFI wrapper auditing. Main TCC/LLVM `2431`/`2429`; negative `773`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.80 (2026-11-03): direct 32-bit x86 stdcall. LLVM now emits `x86_stdcallcc` for `@stdcall` symbols on i386 Linux and Windows. The Windows COFF `_name@N` decoration is derived by LLVM from the function type (verified: `take_i32`/`take_i64`/`take_pair` emit `_take_i32@4`/`_take_i64@8`/`_take_pair@8`), including `sret` and `byval` shapes. Direct call instructions carry the callee convention; a direct function-address indirect call keeps it. `SAFE-FFI-006` remains fail-closed for variadic `@stdcall` and non-x86 32-bit targets; 32-bit Windows callbacks remain rejected by `SAFE-FFI-005` until callback convention metadata exists. Added positive i386-Windows stdcall target coverage and negative ARM32/variadic coverage. Main TCC/LLVM `2431`/`2429`; negative `773`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.81 (2026-11-03): 32-bit Windows cdecl callbacks + cstd audit. `FunctionTypeData` now records stdcall as part of function-type identity, so a `@stdcall` function declaration cannot be passed where a cdecl callback is expected. On 32-bit Windows, the default cdecl callback convention matches the internal scalar/pointer ABI, so `signal`/`atexit`/`at_quick_exit`/`call_once`-style callbacks are accepted; stdcall callback conversion remains rejected by `SAFE-FFI-005`. Audited and migrated `cstd/windows`: stdio caches are thread-local, extern calls are inside the audited wrappers, raw-pointer `read`/`write`/`popen`/`pclose`/`MoveMemory`/`FillMemory`/`ZeroMemory` are `unsafe func`, and `mkdir` matches the POSIX unsafe surface. Added an i386-Windows target check that imports `cstd` and asserts no `SAFE-*` diagnostics, plus cdecl/stdcall callback coverage. Main TCC/LLVM `2431`/`2429`; negative `774`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.82 (2026-11-03): Windows platform safety audit. Fixed the duplicate `InitializeSRWLock`/`AcquireSRWLockExclusive`/`TryAcquireSRWLockExclusive`/`ReleaseSRWLockExclusive`/`SRWLOCK_SIZE` declarations between `std/win/mutex.ch` and `std/win/rwlock.ch`, which blocked every i386 Windows target build. Audited Windows `std` synchronization/time/OsString/thread-pool wrappers and the test harness: extern calls and raw-pointer operations are now inside explicit audited `unsafe` boundaries, stdio caches are thread-local, and raw-pointer APIs are marked unsafe. Audited `async/win/reactor.ch` (pipe creation, reactor readiness, poll probing) and made `async::pipe` unsafe to match POSIX. The i386 Windows target now passes `--check` with no `SAFE-*` diagnostics for `cstd`, `std`, `fs`, `net`, `test`, and `async`; added a regression check importing those modules. Remaining platform work: `net/win` and `window/win` migrations. Main TCC/LLVM `2431`/`2429`; negative `774`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.83 (2026-11-03): complete i386 Windows library safety pass. Wrapped the Windows `net/win` IOCP/platform API, `fs/win` filesystem/UTF-8/metadata wrappers, `window/win`, and `webview/win` bodies in explicit audited unsafe boundaries; fixed the 32-bit `size_t` file-length shift (`nFileSizeHigh << 32` now goes through `u64`), added `Zeroable` for `WNDCLASSEXW`, and added the missing unsafe boundary for `osrand`'s `BCryptGenRandom`. The i386 Windows target now passes `--check` with no `SAFE-*` diagnostics for `cstd`, `std`, `fs`, `net`, `test`, `async`, `tls`, `window`, and `webview`; the negative regression imports all of them. Main TCC/LLVM `2431`/`2429`; negative `774`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.84 (2026-11-03): reviewed audit baseline update. The Windows platform migration adds `unsafe_blocks 3775 -> 3780` and `unsafe_values 4915 -> 4921`: `net` Windows error branches, `osrand`'s `BCryptGenRandom` call, and the new i386-Windows target regression test. All are intentional audited boundaries; `scripts/safety-audit.sh --update` was run and the baseline diff was reviewed. Main TCC/LLVM `2431`/`2429`; negative `774`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth against the new baseline.

*Revision note v4.85 (2026-11-03): stdcall callback types. `FunctionTypeData` now records stdcall for both declarations and `@stdcall type` aliases; the parser applies the annotation after the right-hand side and MIR interns the flag on function types. LLVM emits `x86_stdcallcc` for indirect calls through a stdcall function pointer and keeps the direct-call/declaration convention; `SAFE-FFI-005` now accepts scalar/pointer stdcall callbacks on 32-bit x86. The C emitter prefixes function-pointer types, indirect calls, and definitions with `__chem_stdcall` (verified with `-m32 -fsyntax-only`), so prototypes and definitions agree. `SAFE-FFI-006` remains fail-closed for variadic stdcall and non-x86 32-bit targets. Added positive i386-Windows stdcall-callback coverage, a cdecl/stdcall type-mismatch negative, and IR/C checks. Main TCC/LLVM `2431`/`2429`; negative `774`; safety `74`; async `70`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.

*Revision note v4.86 (2026-11-03): drop elaboration. Local/parameter/async-frame aggregates with generated glue now carry per-field/per-element drop trees; async frame flags are precomputed into the frame type and materialized in poll/drop/ramp functions, so normal completion, cancellation, and resume cleanup agree. Async destructible parameters are registered for normal completion, and a moved frame field is accepted (the old `SAFE-MOVE-004` rejection is now a positive test). Fixed array fields are tracked recursively per element, including partial element initialization and reassignment; async C drop text and LLVM MIR both emit the same field tree. Added runtime coverage for live/moved frame fields, live struct parameters, nested array whole/partial/reassignment, and cancellation. Main TCC/LLVM `2431`/`2429`; negative `774`; safety `74`; async `75`; libs `732`; plugins `1305`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth after a reviewed `+8 unsafe_blocks`/`+4 raw_pointer_addrs` test-only baseline bump (`unsafe_blocks 3788`, `unsafe_values 4921`).

*Revision note v4.87 (2026-11-03): raw escape after a field move. `MIRDestructible` now records `field_moved`; `mark_raw_escaped` keeps field-wise drop flags when a field (or the whole value) was already moved, instead of switching to a whole-value drop that would double-drop the moved field. Raw writes into a moved-from field can still leak, but cannot double-drop. Added a runtime regression for `&raw mut` after a field move. Final drop-elaboration gates: main TCC/LLVM `2431`/`2429`; negative `774`; safety `74`; async `76`; libs `732`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth against the reviewed test-only baseline (`unsafe_blocks 3790`, `unsafe_values 4921`).
*Revision note v4.88 (2026-11-03): semantic FFI wrapper audit, tranche 1. Allocator ownership is now an explicit unsafe boundary: `GlobalAllocator.dealloc/grow/shrink` and `dealloc_bytes` require `unsafe`; zero-sized allocations are freeable; grow/shrink to zero returns `AllocError.InvalidLayout` instead of `realloc(ptr, 0)`, which can free while returning an error; debug tracking no longer dereferences a null allocation. Filesystem wrappers validate null paths/handles/buffers, use checked path-length arithmetic, and reject `atomic_write` temp paths that would overflow the fixed stack buffer; POSIX temp-file creation bounds both the template and the caller output. Socket wrappers reject null buffers with positive lengths, negative send lengths, lengths outside signed `int`, null addresses, and ports above 65535; the port conversion buffer is bounded. Added allocator zero-size/grow-to-zero, null filesystem input, near-`PATH_MAX` atomic-write, and invalid socket port/buffer tests. Full audit matrix and remaining modules: `lang/docs/safety/ffi-wrapper-audit.md`. Main TCC/LLVM `2433`/`2431`; negative `775`; safety `74`; async `77`; libs `734`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth after a reviewed `unsafe_func_decls 36 -> 40` and test-only raw-pointer baseline update.
*Revision note v4.89 (2026-11-03): semantic FFI wrapper audit, tranche 2 (ownership wrappers and strings). Zero-sized `Box<T>` values now allocate one byte, so `Box::box` never produces a null pointer that `get`/`delete` would mishandle. `DynBox`'s `fat`/`data`/`drop` fields are internal rather than safe-code constructible, preventing a caller from pairing an arbitrary data pointer with a mismatched or null drop function. String append entry points reject null sources before `strlen`/`memcpy`, while zero-length appends remain no-ops. Added a zero-sized Box runtime test, a `DynBox` field-visibility negative test, and a null-append runtime-panic test. Full audit matrix and remaining modules: `lang/docs/safety/ffi-wrapper-audit.md`. Main TCC/LLVM `2433`/`2431`; `@test` `57`; negative `776`; safety `75`; async `77`; libs `734`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth after a reviewed `panic_calls 113 -> 116` test/validation baseline update.
*Revision note v4.90 (2026-11-03): semantic FFI wrapper audit, tranche 3 (collections and reference counts). `vector<T>` constructors now fail closed for zero-sized element types instead of creating a vector whose indexing/pointer arithmetic would be invalid. `Rc`/`Weak` and `Arc`/`ArcWeak` check count overflow before increments so a wrapped count can never free storage while a handle remains. `u16_strlen` is now an unsafe raw-pointer utility; its internal caller uses an explicit unsafe boundary. Added a zero-sized-vector runtime-panic test and a `u16_strlen` unsafe-call negative test. Full audit matrix and remaining modules: `lang/docs/safety/ffi-wrapper-audit.md`. Main TCC/LLVM `2433`/`2431`; `@test` `57`; negative `777`; safety `76`; async `77`; libs `734`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth after a reviewed `panic_calls 116 -> 124`, `unsafe_func_decls 40 -> 41`, and `unsafe_values 4921 -> 4922` baseline update.
*Revision note v4.91 (2026-11-03): semantic FFI wrapper audit, tranche 4 (thread-pool Future ownership). `concurrent.Future<T>` now keeps its promise pointer private, makes the raw-pointer constructor `unsafe`, and panics on `get` after the future was emptied instead of dereferencing null. `concurrent.Promise<T>` fields and constructor are internal to `std`, so safe callers cannot manufacture or mutate an unsynchronized promise. Added a negative test for the unsafe Future constructor. Main TCC/LLVM `2433`/`2431`; `@test` `57`; negative `778`; safety `76`; async `77`; libs `734`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v4.92 (2026-11-03): semantic FFI wrapper audit, tranche 5 (vector replacement and filesystem buffers). `vector::set` now destroys the old element before storing the replacement, so destructor-bearing elements are not leaked. `fs::temp_dir` rejects null/undersized output buffers, `fs::copy_directory` bounds child path construction, and Windows `read_dir` bounds its search pattern. Added a destructor-counter vector replacement test and a temp-dir capacity/null test. Main TCC/LLVM `2434`/`2432`; `@test` `57`; negative `778`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth after a reviewed `destruct_stmts 23 -> 27` and `raw_pointer_addrs 10035 -> 10036` baseline update.
*Revision note v4.93 (2026-11-03): large-array drop elaboration. Fixed arrays of directly destructible elements larger than 64 now use one runtime bool flag per element instead of falling back to a whole-array flag, so constant-index partial initialization, replacement, and scope cleanup work for `[100]string`, `[100]custom_dtor`, and similar arrays. Smaller arrays keep the existing scalar-flag form; arrays whose element type has generated drop glue, async frame slots, and unsized arrays remain unchanged. Added a 100-element destructor-counter regression test and a positive `[100]DT` partial-initialization negative test. Main TCC/LLVM `2435`/`2433`; `@test` `57`; negative `779`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v4.94 (2026-11-03): HRTB large reference arrays. A top-level fixed array of direct references with more than 64 elements now uses one wildcard element step in the MIR region signature instead of one path per element. The borrow checker seeds a single element-loan region for the array, so a callback returning an element keeps the caller source live and a later mutable borrow of any element still conflicts. Struct-nested large arrays, arrays of aggregates, and unsized arrays remain explicit `SAFE-BORROW-099` rejections. Added positive and alias-conflict tests for `[65]&'a int`. Main TCC/LLVM `2435`/`2433`; `@test` `57`; negative `782`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v4.95 (2026-11-03): custom `@scoped_callback` sinks. `SAFE-SCOPED-001` now verifies an unannotated custom sink by body-checking it with the same forwarding contract instead of requiring every sink to be on the fixed library list. A custom sink may discard the callback or forward it to a known/checked sink; a body that stores, returns, captures, passes it elsewhere, has no body, or forms a recursive cycle is rejected. Added positive custom-sink and nested-known-sink tests plus a negative storing-custom-sink test. Main TCC/LLVM `2435`/`2433`; `@test` `57`; negative `783`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v4.96 (2026-11-03): conditional auto-trait bounds through generic parameters. `generic_negative_impl_applies` now treats a generic-parameter argument as satisfying a conditional impl's bound when the parameter's own declared/where-clause bits imply it, and `type_satisfies_unpin` honors `T: Unpin` for a bare generic parameter. This lets a generic body with `T: Unpin` use a `Wrapper<T>` whose positive `Unpin` impl is conditional on `T: Unpin`, while the same body without the bound remains rejected. Added positive and negative compile tests. Main TCC/LLVM `2435`/`2433`; `@test` `57`; negative `785`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v4.97 (2026-11-03): large generated-glue array drops. Large fixed arrays whose element type has generated drop glue now use a `MIRDropField` element template with one runtime bool-flag array per leaf field instead of falling back to a whole-array flag. Whole-element and per-field constant-index initialization/replacement drop only the initialized leaves; the uninitialized sibling field is never destroyed. Nested arrays inside an element, async frame slots, and unsized arrays remain on the whole-field fallback. Added 100-element whole-element and partial-field drop-counter tests plus a partial-field compile test. Main TCC/LLVM `2437`/`2435`; `@test` `57`; negative `785`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v4.98 (2026-11-03): caller-owned retaining slots. A retaining reference slot reached through a caller-owned reference parameter no longer requires its outer borrow region to outlive the stored region. The slot's outer borrow only has to cover the call; the stored reference's own declared region must be outlived by every source. This accepts `slot: &'o mut &'i T, x: &'i T` without a `where 'o: 'i` edge while still rejecting distinct source/stored regions and local sources. Added a positive type-proved retaining-slot test. Main TCC `2437`; negative `786`; safety `76`; async `77`; audit no change.
*Revision note v4.99 (2026-11-03): mismatched retaining-pointee binders. A retaining pointee may now use different binders from the callback (`&'a mut &'b T`), not only the same binder (`&'a mut &'a T`). Every nested reference must still use a binder of that function type, and the call-site retaining rule checks every source against every stored region, so a shorter source cannot be stored into a longer caller slot without the caller's outlives edge. A free or `'static` pointee lifetime remains `SAFE-BORROW-099`. Added positive local-slot and caller-slot tests plus a distinct-source negative. Main TCC `2437`; negative `789`; safety `76`; async `77`; audit no change.
*Revision note v5.00 (2026-11-03): remaining HRTB precision. Large direct-reference arrays are now supported inside one direct struct field: MIR records a `[field, wildcard]` path, the borrow checker keys the wildcard element loan by `(root, field index)`, and element loads resolve the same key through field projections. Arrays nested more than one aggregate level, arrays of aggregates, and unsized arrays remain `SAFE-BORROW-099`. Retaining pointees may use distinct binders, `'static`, or a free lifetime carried by a direct reference parameter; the call-site rule still checks every source against every stored region. Opaque retaining storage with no named region is now fail-closed for caller-owned slots: the multi-borrow equal-region shortcut no longer accepts it, closing a dangling-store hole. Added positive/negative tests for struct large arrays, static/free pointee lifetimes, and opaque retaining storage. Main TCC/LLVM `2437`/`2435`; negative `796`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.01 (2026-11-03): nested arrays in large drop templates. A large generated-glue array's element template can now contain fixed arrays of up to 64 elements: each inner leaf gets its own outer-index bool-flag array, and `destroy_drop_fields_at` offsets the template base by the inner `array_index` before reading the flag/dropping the element. Whole-element initialization/destruction covers nested arrays of directly destructible elements and nested struct-array leaves without dropping uninitialized siblings. Inner arrays larger than 64, large arrays in async frame slots, and unsized arrays remain on the whole-field fallback. Added runtime drop-counter tests for `[100] { inner: [2]DT }` and `[100] { inner: [2]{a:DT,b:DT} }`. Main TCC/LLVM `2439`/`2437`; `@test` `57`; negative `796`; safety `76`; async `77`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.02 (2026-11-03): drop elaboration for large inner arrays and async frame slots. Inner fixed arrays up to 1024 elements inside a large template element now expand into outer-index leaf flag arrays; small outer arrays whose element is itself a large array carry one flag block per element, and whole-element assignment/destruction arms or drops the whole block (`MIRDropElementRef::flag_array`/`whole_children`). Directly destructible large arrays in async frame slots use a frame-resident bool-flag block (`frame_flag_array`), added to both the C frame struct and the MIR frame type, so partial initialization and cancellation drop exactly the live elements. Generated-glue large arrays in async frames, inner arrays above 1024, and unsized arrays remain on the whole-field fallback. Added TCC/LLVM runtime tests for `[100]{inner:[65]DT}`, small-outer/nested-large arrays, and partial large arrays across normal completion/cancellation. Main TCC/LLVM `2442`/`2440`; `@test` `57`; negative `796`; safety `76`; async `80`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.03 (2026-11-03): generated-glue large arrays in async frames. A frame array slot whose element has generated glue now uses an element-template drop tree with frame-resident leaf flag blocks, instead of a whole-slot flag. The frame C drop text emits an outer-element loop and indexes each leaf flag by that element; MIR drop lowering calls `destroy_array_template` for the template root. Partial element initialization and cancellation therefore drop exactly the initialized leaves. Large directly-destructible arrays, generated-glue array slots, and small-outer/nested-large arrays are covered; inner arrays above 1024, generated-glue large array fields inside a frame aggregate, and unsized arrays remain on the whole-field fallback. Added completion coverage for a `[100]{a:DT,b:DT}` frame array. Main TCC/LLVM `2442`/`2440`; `@test` `57`; negative `796`; safety `76`; async `81`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.04 (2026-11-03): generated-glue large array fields in frames. A generated-glue large array **field** inside an async frame aggregate now uses the same element-template tree as a slot: the frame C drop text resolves the field name, loops over outer elements, and indexes each leaf flag by that element; MIR drop lowering resolves the field address before `destroy_array_template`. Whole-field construction/assignment arms every inner leaf and destruction drops only live leaves. Partial element assignment through such a field is still rejected by initialization tracking (`SAFE-INIT-001`). Inner arrays above 1024 and unsized arrays remain on the whole-field fallback. Added a `[65]FramePair` field completion test. Main TCC/LLVM `2443`/`2441`; `@test` `57`; negative `796`; safety `76`; async `82`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.05 (2026-11-03): flat flag blocks for very large inner arrays. Inner arrays above 1024 inside a template element no longer expand one child per index: they use one flat bool-flag block with `inner_count` and `outer*inner` slots. `set_drop_fields_at` arms a contiguous slice with `memset_ptr`; `destroy_drop_fields_at` uses `destroy_flag_array_from` with an outer-index base and a runtime inner loop; frame C drop text emits the matching `outer*inner + inner` flag index. This supports `[N][M]DT` and struct elements with `[M]DT` fields for large M without unbounded template-tree growth. Added a 1025-element inner-array runtime test. Unsized arrays remain on the whole-field fallback; partial nested element assignment remains `SAFE-INIT-001`. Main TCC/LLVM `2443`/`2441`; `@test` `57`; negative `796`; safety `76`; async `82`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.06 (2026-11-03): array-field element initialization and address-path casts. The MIR move checker now propagates address paths through `Cast`, which is emitted before `IndexAddr` when a struct field array is indexed; a two-operand aggregate store through a derived element address initializes that element path. `drop_flag_for_path_index` now returns the element's whole child tree (or flag block) and `MIRDropElementRef` carries the field path so lowering can re-resolve after RHS lowering. Constant-index writes such as `h.items[0] = ...` now read back and drop correctly for small and large generated-glue array fields. A direct nested fixed-array write (`arr[0][0] = ...`) is explicitly rejected with `SAFE-INIT-001` rather than silently leaking or mis-dropping, and vector/slice pointer-backed writes remain allowed (the check only applies to fixed-array bases). Added positive small/large field-element tests, a frame field-element test, and a nested-array negative. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.07 (2026-11-03): nested fixed-array element assignment. `arr[outer][inner] = ...` now resolves the outer element's inner flag block (`drop_flag_for_nested_index`), including scalar per-inner-element flags and whole-child trees, and `MIRDropElementRef` carries the outer/inner indices for re-resolution. `destroy_array_drop_fields` now routes an element whose type is itself a fixed array through `destroy_array_drop_fields` instead of treating its children as struct fields, which fixes invalid generated C and exact inner-element drops. Added `[2][2]` and `[2][65]` partial nested-element runtime tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.08 (2026-11-03): checked string_view numeric parsing. `to_i64`/`to_u64` now accumulate magnitude with explicit limits and return `Err("integer overflow")` instead of wrapping; signed minimum is handled without an overflowing negation. Narrow parsers (`to_i8/i16/i32/u8/u16/u32`, and therefore `to_int`/`to_uint`) range-check before casting and return `Err("out of range")`. Added limit and one-past-limit tests for every width, including `i64::MIN` and `u64::MAX`. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.
*Revision note v5.09 (2026-11-03): thread-pool promise result slot. `concurrent.Promise<T>` no longer copies an uninitialized `T` into its result field: `val` is an `Option<T>` initialized to `None`, the worker stores `Some(result)`, and `Future::get` uses `Option::take` before deleting the promise. This removes destructor calls on garbage bytes (and the resulting double-free/leak for `string`-like results). Added `test_pool_future_drops_result_once`, which submits a destructor-bearing result and checks it is dropped exactly once. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.10 (2026-11-03): safe containerized string splitting. `string_view::split` and `string::split` previously returned `vector<string_view>`; because the container hid the source loan, returning the vector from a local source compiled and yielded dangling views. Both safe forms now return owned `vector<string>` copies. The zero-copy path moved to `split_views`, which is `unsafe` and requires the caller to keep the source alive. Updated `refgen` to use the owned form and added three positive split tests plus `neg_split_views_requires_unsafe`. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.11 (2026-11-03): u16string append aliasing and null sources. `u16string.append_with_len` now rejects a null source for a positive length and recomputes a self-aliased source address after `ensure_mut` may move the storage (the SSO -> heap transition previously copied from the overwritten union). `u16string.append_utf8_view` rejects a null source. Added the self-append storage-move regression and two runtime panic tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.12 (2026-11-03): associated-type parameter projection and inherited object safety. Abstract interface methods may now use `Self::Item` in parameter position: the declaration has no body, and static dispatch through the concrete implementation supplies the real parameter type. Default-method bodies still reject the projection because they lower against the interface prototype. Object safety now rejects any interface that declares or inherits an associated type, so `dyn HasItem` and `dyn Child` are fail-closed instead of exposing placeholder vtable slots. Added static-dispatch and generic round-trip tests plus direct/inherited dyn negative tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.13 (2026-11-03): slice rest condition patterns. `[T]`/`&[T]` condition patterns now accept one `..` (for example `if(var [first, .., last] = view)`). Names before `..` use constant indices; names after it use the runtime slice length (`size - offset`), with the existing `size >= name_count` guard. Fixed-array patterns are unchanged. Added first/last, end-only, start-only, and too-short runtime tests; the old rejection test is now a positive compile test. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.14 (2026-11-03): Box<[T]> and &mut [T] condition patterns. Condition patterns now accept `&mut [T]` and `Box<[T]>`, including one `..`; `Box<[T]>` reads its backing `_values` vector's data/size without copying the owning vector, so Copy elements can be matched exactly or by first/last. Statement-form slice patterns and else paths remain unsupported. Added mutable-slice and Box exact/rest runtime tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.15 (2026-11-03): runtime-checked statement slice patterns. `var [a, b] = view`, `var [first, .., last] = view`, and the same forms for `&mut [T]` and `Box<[T]>` now lower. The slice/box form emits the existing exact/rest length condition and aborts through `__chc_bounds_fail` when it does not hold, then binds each Copy element using the same slice/box data path as condition patterns. Fixed-array statement patterns keep their compile-time length check; `else` paths remain unsupported. Added shared-slice, mutable-slice, and Box statement-pattern runtime tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.16 (2026-11-03): divergent else paths for slice patterns. `var [a, b] = view else return ...` (and `break`/`continue`/`unreachable`) now lowers for `[T]`, `&mut [T]`, and `Box<[T]>`. The runtime length condition branches to the else block on mismatch; without else it keeps the existing bounds-fail abort. Parser reuses one `parsePatternElse` helper. Added match/mismatch runtime tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.17 (2026-11-03): bare `unreachable` is an unsafe boundary. A user-written `unreachable` statement now reports `SAFE-UNSAFE-002` outside `unsafe`; `unsafe { unreachable }` still lowers to the trap. Compiler-generated for-loop placeholders are marked generated and remain no-ops. MIR now lowers bare `UnreachableStmt` (previously it was an unsupported statement kind) and skips generated placeholders. Pattern `else unreachable` branches are unchanged for now; they are exhaustiveness branches, not bare trap statements. Added negative/positive compile tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `802`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.18 (2026-11-03): pattern `else unreachable` is an unsafe boundary. The parser accepts `else unsafe { unreachable }` (and the concise `else unsafe unreachable`) and records the explicit boundary on the pattern else branch; a safe `else unreachable` now reports `SAFE-UNSAFE-002`. All in-tree `.ch` uses were migrated (178 files, 1191 code occurrences; comments untouched). The CBI `set_else_unreachable` helper marks plugin-generated traps unsafe because generated-AST plugins cannot spell an unsafe block. Added negative/positive compile tests. Main TCC/LLVM `2478`/`2476`; `@test` `57`; negative `804`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.19 (2026-11-03): `T::Item` generic associated-type projections. A qualified type whose first segment is a generic parameter now resolves its associated declaration through inline bounds, `where` clauses, and inherited interfaces; the concrete implementation binding is substituted during monomorphization, including generic struct fields, nested generic containers, and generic impl bindings that project a type parameter (`type Item = T::Item`). Generic impls are demanded before the associated binding is looked up. Generic-master fields that still contain a projection are not rejected as void, and the link phase pre-links where-clause bounds before signature types. Added runtime coverage for inline/where/inherited bounds, two concrete item types, generic struct fields, nested `vector<T::Item>`, and generic impl binding projection, plus unbounded/unknown-member negative tests. Main TCC/LLVM `2479`/`2477`; `@test` `57`; negative `806`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.20 (2026-11-03): associated-type bounds. Interface declarations now accept `type Item: Bound` (including `+` chains); the bound is linked and every implementation binding is checked with the same trait-satisfaction machinery as generic constraints. A non-Copy binding for `type Item: Copy` is rejected with `associated type 'Item' binding does not satisfy bound 'Copy'`. Generic impl bindings that still project a type parameter defer the check until concrete resolution. Added positive and negative compile tests. Main TCC/LLVM `2479`/`2477`; `@test` `57`; negative `808`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1931`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.21 (2026-11-03): inline methods on tuple structs. `struct Point(int, int) { func sum(&self) : int { ... } }` now parses and compiles; the body uses the normal struct member parser and supports generic tuple structs plus method-level `where` clauses. Tuple wildcard patterns were already implemented and are now documented as such. External inherent impls on tuple structs and `impl Trait for (T1, T2)` remain open. Added runtime coverage for concrete and generic tuple-struct methods. Main TCC/LLVM `2480`/`2478`; `@test` `57`; negative `808`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1932`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.22 (2026-11-03): nested tuple patterns and generic-impl lookup determinism. Tuple patterns now accept nested groups (`var ((a, b), c) = value`) and wildcards at any level. Each tuple level is arity-checked; a nested position whose value is not a tuple is rejected. MIR binds nested leaves through their full tuple-field path and clears the matching nested drop flags, so destructible leaves still drop exactly once. The generic-impl method lookup path now carries the target struct's parameter aliases during generic body finalization, and the alias pointer is thread-local so concurrent instantiations cannot observe another thread's map. This removes the intermittent `unresolved child 'read'/'show'` failures in generic impl bodies. Added nested destructuring and nested-drop tests plus nested arity/type negatives. Main TCC/LLVM `2482`/`2480`; `@test` `57`; negative `810`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1934`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.23 (2026-11-03): tuple patterns in `if` conditions. `if(var (a, b) = value)` and nested forms (`if(var (a, (b, c)) = value)`) now parse through the shared tuple-pattern parser and lower through the existing irrefutable tuple path. The same arity and nested-type checks apply as in statement position. Added a nested condition-pattern runtime test. Main TCC/LLVM `2483`/`2481`; `@test` `57`; negative `810`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1935`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.24 (2026-11-03): external inherent implementations. `impl Point { ... }` now parses as an inherent implementation (no interface) for structs, unions, variants, and tuple structs; methods are indexed on the target container and lowered through the existing impl path. Name mangling handles the missing interface, generic visitors skip the absent interface type, and interfaces are rejected as inherent targets. Generic inherent implementations remain unsupported (`generic implementation target is not an interface`). Added tuple-struct external-method runtime coverage and an interface-target negative. Main TCC/LLVM `2484`/`2482`; `@test` `57`; negative `811`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1936`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.25 (2026-11-03): generic inherent implementations. `impl<T> Pair<T> { ... }` now indexes its methods on the concrete target when instantiated, the same way a concrete inherent impl does; no interface/vtable registration is involved. Generic inherent methods can use `T` and method-level `where` bounds. Added runtime coverage for a generic tuple struct with external inherent methods. Main TCC/LLVM `2485`/`2483`; `@test` `57`; negative `810`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1937`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.26 (2026-11-03): nested fixed-array patterns. `var [[a, b], [c, d]] = matrix` and mixed forms such as `var [_, [b, c], _] = matrix` now work for fixed arrays; each level is arity-checked and nested positions must themselves be fixed-size arrays. MIR reads nested leaves through their full array-index path. Nested groups are exact; `..` inside a nested group is rejected with an explicit diagnostic, while the existing top-level rest forms remain. Added runtime coverage (statement and `if` condition) plus nested-arity and nested-non-array negative tests. Main TCC/LLVM `2486`/`2484`; `@test` `57`; negative `812`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1938`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.27 (2026-11-03): associated-type equality constraints. Trait bounds now parse `Interface<Item = T>` (including lifetime arguments and nested generic RHS types) and carry the bindings on `GenericType`. Equality is enforced during concrete instantiation of generic functions, structs, unions, variants, interfaces, impls, and type aliases: the concrete impl's associated binding must exactly match the bound target. Equality targets that are still generic parameters are deferred to the outer instantiation. The declaring interface may be inherited, and an unknown associated name or an equality binding on a non-interface type is rejected at link-signature time. Added positive coverage for inline, `where`, generic-struct, and inherited-interface equality, plus mismatch, unknown-name, and non-interface negatives. The parser treats `=` as an equality only outside a pending nested `>>`, preserving `var f : Fn<(x) => R<int>> = ...`. Main TCC/LLVM `2486`/`2484`; `@test` `57`; negative `816`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1938`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.28 (2026-11-04): safety-parity priority and map `find` ownership. The roadmap now explicitly freezes feature-parity work: remaining language-surface items are not release blockers unless a concrete safe API needs them. Essential work is the stdlib/FFI wrapper audit, move/borrow/init closure, async/concurrency, no-panic effects, and cross-target safety. In the wrapper audit, `unordered_map::find` and `ordered_map::find` copied a stored value into the caller slot without requiring `Value: Copy`; a `D` value with a destructor was duplicated bitwise and double-freed (verified with a heap pointer). Both now require `Value: Copy` and reject owning values with the standard where-clause diagnostic; callers that need a borrowed view use `get_ptr`, and mutation remains explicit/unsafe. Added unordered/ordered-map negative coverage. Main TCC/LLVM `2486`/`2484`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1938`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.29 (2026-11-04): worker-owned thread-pool teardown. A safe task could capture the last `Arc<ThreadPool>`; dropping it on a pool worker made the destructor join itself and free `PoolData` while the worker still used it (reproduced as heap corruption). `PoolData` now stores each worker's OS thread id and the destructor distinguishes an owner teardown from a worker teardown. Owners join all workers then free; a worker sets shutdown and defers, and the last worker drains the queue, joins the remaining handles, and frees the block exactly once. `ThreadPool::delete` now takes `&mut self` and the worker path no longer self-joins. Added a runtime regression where the last `Arc<ThreadPool>` is dropped inside a submitted task. Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `735`; interpret `1938`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.30 (2026-11-04): POSIX filesystem bounds/null and UTF-16 validation. `fs::read_dir` assembled `path/name` into a fixed `PATH_MAX_BUF` stack buffer without checking either component, so a long path plus entry name could overflow the stack; it now rejects null paths and checks the separator/name/NUL arithmetic before every write. `fs::metadata(null)` now returns `InvalidInput` instead of passing null to `lstat`. The UTF converters reject null inputs and `utf16_to_utf8` rejects unpaired high/low surrogates instead of emitting invalid UTF-8 (or reading an unpaired trailing unit as data). Added a filesystem null-input regression. Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `736`; interpret `1938`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.31 (2026-11-04): review-only M7 items. Audited the remaining string-view/u16string raw constructors, Rc/Arc weak-handle paths, and RefCell guard lifetime/escape behavior against their safe contracts. Raw constructors are already unsafe; safe views are lifetime-bound; count/upgrade/try-unwrap paths are covered by runtime and race tests; guard leakage leaves the cell borrowed (safe leak/DoS) rather than creating dangling access. No code change was required. The only remaining wrapper work is Windows UTF-16/tree removal, Windows IPC, TLS/HTTP, and the target-specific audit list; actual zero-sized vector support remains an ergonomic/fail-closed item, not a safety hole. Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `736`; interpret `1938`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.32 (2026-11-04): Windows recursive tree removal. `remove_dir_all_recursive_native` bounded both the original wide path and every `WIN32_FIND_DATAW` name before writing into its fixed `WIN_MAX_PATH` buffers, rejects overflow with `PathTooLong`, owns the child buffer through `CustomDeletor<u16>` (closing a leak), and closes the find handle when the child allocation fails. `utf8_to_utf16_inplace` now rejects null input/output or zero capacity, and the public native directory/file entry points reject null paths instead of passing them to `open`/`mkdir`/`rmdir`/`RemoveDirectoryW`/`DeleteFileW`. Extended the filesystem null-path regression. The i386 Windows target safety check compiles `fs` cleanly; negative `818/818`, TCC libs `736/736`. Main TCC/LLVM `2487`/`2485`; `@test` `57`; safety `78`; async `83`; plugins `1305`; interpret `1938`; `MIRTests` `4749`; audit no growth.*
*Revision note v5.33 (2026-11-04): HTTP body ownership and raw-wrapper boundaries. `Body.make_body` is now an unsafe constructor; when it does not take ownership it copies the already-read bytes into a Body-owned `net::Buffer`, closing the safe-API path where a `Request`/`Response` Body retained a stack parser buffer. `Body.read` rejects a null destination with positive capacity and capacities above `INT_MAX`; `net::Buffer.as_ptr` no longer indexes `read_pos == size` after the header bytes are fully consumed. `ResponseWriter.send_file` NUL-terminates a string-view path before `open`/`CreateFileA`; `strcasecmp`, `parse_request_from_bytes`, and `read_response_incremental` are explicit unsafe boundaries. Added Body-null/copy/strcasecmp regressions and ran them through the LLVM TLS filter. Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `736`; interpret `1938`; `MIRTests` `4749`; audit no growth. The full TLS suite still has pre-existing server-test failures unrelated to this tranche (confirmed with the changes stashed); the LLVM filtered HTTP body/client checks used here pass.
*Revision note v5.34 (2026-11-04): TLS hostname and accepted-socket ownership. `ssl_set_hostname` copied the caller's pointer only by reference; a temporary host string could be freed before SNI/certificate verification. It now copies up to 255 bytes into an SSLContext-owned buffer, tracks ownership, replaces/frees prior copies safely, and `ssl_free` releases it once. `tls_accept` now consumes the accepted socket on both success and failure, and HTTP `handle_conn` uses a single close path that suppresses `ssl_free`'s close before closing the fd itself, removing a double-close/fd-reuse race. Added a hostname-copy runtime regression; LLVM TLS filtered tests `7/7` pass. Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `736`; interpret `1938`; `MIRTests` `4749`; audit no growth. Remaining TLS work: caller-owned config/ALPN lists and full transport lifetime coverage.
*Revision note v5.35 (2026-11-04): TLS caller-owned lists and Windows IPC. Marked `ssl_set_ca_chain`, `ssl_set_own_rsa_key`, `ssl_set_alpn_protocols`, and `ssl_get_alpn_negotiated` unsafe because their safe signatures could not enforce caller lifetime/ownership; added null/list-entry validation. Hardened the Windows test IPC path: null/invalid pipe handles and null buffers with positive lengths are rejected, oversized writes are refused, read counts are clamped before NUL termination, and a failed `ConnectNamedPipe` closes the child process/thread and pipe handles. LLVM TLS filtered tests `7/7`; i386 Windows target safety check `818/818`; TLS/HTTP checks clean. Remaining in item 1: `ssl_set_config` caller-owned config lifetime, actual zero-sized vector support (safe but ergonomic), and target-specific non-Windows OS-gated wrappers. Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `736`; interpret `1938`; `MIRTests` `4749`; audit no growth.
*Revision note v5.36 (2026-11-04): config ownership and TLS/Windows closure. `ssl_set_config` now copies the caller's `SSLConfig` into SSLContext-owned storage, so stack configs and temporary `ssl_config_init(...)` expressions are safe; the internal tls_accept/HTTP client heap-config allocations were removed. The raw CA/private-key/ALPN pointers inside a config are still caller-owned and their setters are unsafe boundaries. With Windows IPC bounds/cleanup in v5.35, item 1 now leaves only actual zero-sized vector support (safe but ergonomic) and target-specific non-Windows OS-gated wrapper coverage. LLVM TLS filtered tests `11/11` plus TLS 1.3 RSA E2E pass; i386 Windows target safety `818/818`; TLS/HTTP checks clean. Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `736`; interpret `1938`; `MIRTests` `4749`; audit no growth.
*Revision note v5.37 (2026-11-04): macOS cstd and POSIX reactor. Unsafe-gated the Darwin `__error`/stdio-global/`_NSGetEnviron` wrappers, added fd_set bounds checks, made the select fallback report `POLLERR` for fds ≥1024, and checked epoll/kqueue scratch allocation sizes/nulls with select fallback. Target checks for `x86_64-apple-darwin` (and arm64 roots) are clean for async/std/fs/net/tls/http/server/test; Linux async `83/83`. Item 1 now leaves only actual zero-sized vector support (safe but ergonomic) and platforms that still lack a complete cstd source mapping (FreeBSD). Main TCC/LLVM `2487`/`2485`; `@test` `57`; negative `818`; safety `78`; async `83`; plugins `1305`; TCC libs `736`; interpret `1938`; `MIRTests` `4749`; audit no growth.
*Revision note v5.38 (2026-11-05): zero-sized vectors and UTF-8 lookahead. `vector<T>` now supports `sizeof(T) == 0` with a one-byte non-null sentinel, no `realloc` on capacity growth, and per-element destructor execution; the former fail-closed panic test is now a positive runtime test with a destructible zero-sized element. `fs::utf8_to_utf16` measures the NUL-terminated input once and bounds every 2/3/4-byte continuation lookahead by `in_len - i` instead of relying on short-circuit NUL checks. `u16string.append_u16_unit`/`append_codepoint`/`to_utf8` reject length arithmetic that would wrap before `ensure_mut`/`reserve`. TCC main `2489`, LLVM main `2487`, safety `78/78`, TCC libs `736/736`, negative `818/818`; UTF converter covered by a temporary public harness on both backends. Audit baseline was reviewed and updated for the ZST generic instantiations plus the previously landed TLS/HTTP unsafe setter boundaries: `unsafe_blocks 3821`, `unsafe_values 4961`, `unsafe_func_decls 52`, `panic_calls 142`, `raw_pointer_types 9416`. Item 1 now leaves only `string_view`/`u16string` parser coverage and target-source coverage without a complete cstd mapping, plus the final audit-backlog sign-off.
*Revision note v5.39 (2026-11-05): item-1 closure for claimed targets. Added `cstd/posix_bsd` (FreeBSD `__error`, `__stdinp`/`__stdoutp`/`__stderrp`, `environ`) and mapped it from `chemical.mod`; `link "c"` now covers FreeBSD. Removed the duplicate `GetCurrentThreadId` declaration from `std` (cstd owns the Win32 binding). Hardened Windows IOCP: allocation failures panic instead of null-dereferencing, recv/send validate null/length before posting, registration failure completes the future as an error, and `async_iocp_recv`/`async_iocp_send` are explicit unsafe boundaries. Wrapped the Windows-only HTTP `send_file`/static-path/IOCP server branches in audited unsafe blocks and made IOCP context-buffer allocation fail closed by closing the accepted socket. Target regressions now use strict `--target ... --check` and include `http`/`server`; Windows i386/x64, macOS x64/arm64, and FreeBSD x64/arm64/armv7 roots are clean. TCC/LLVM negative `820/820`; audit baseline reviewed at `unsafe_blocks 3824`, `unsafe_values 4966`, `extern_decls 874`, `panic_calls 142`. Item 1 is complete for the claimed roots; remaining target work is non-claimed OS mappings and the Linux/Windows-only window/webview backends.
*Revision note v5.40 (2026-11-05): item-2 first closure slice — non-Copy fixed-array element moves. `var e = arr[0]`, `return arr[0]`, a by-value aggregate argument, and aggregate field/array-literal initialization now move a destructor-bearing element out of an owned local fixed array: the lowering clears the element’s scalar flag, indexed flag slot, or generated field/leaf flags and emits `MoveOut` so the MIR dataflow marks the source element moved. Non-Copy fixed-array destructuring of an owned local now moves each bound element the same way, while wildcard/rest elements remain owned by the source. The front end accepts only a constant index into a mutable local/async-frame fixed array; dynamic indices, `const` bindings, array parameters (passed as a pointer to caller storage), temporaries, array fields, borrowed/slice views, and nested non-Copy patterns stay `SAFE-MOVE-001`/`SAFE-MOVE-004`. Added TCC/LLVM runtime drop-count tests for direct element moves, generated-glue element fields, returned elements, and non-Copy destructuring, plus negative coverage for const/parameter/temporary/field/dynamic/nested shapes. TCC/LLVM negative `828/828`; TCC/LLVM safety `83/83`.
*Revision note v5.41 (2026-11-05): temporary/expression field moves. `make().field` and nested `make().inner.field` now recover the materialized temporary from the lowered `FieldAddr` root, register it as a block-scope destructible with its generated field flags, clear the moved field’s flag, and drop the remainder at the enclosing block end. The old `SAFE-MOVE-004` rejection for this safe shape is now a positive runtime test; user-destructor temporaries and pointer/dereference bases still reject. This deliberately extends a moved temporary’s drop point from statement end to block end (no leak/double-drop; destructor timing is later). TCC/LLVM negative `829/829`; TCC/LLVM safety `84/84`.
*Revision note v5.42 (2026-11-05): array-field element moves. A constant-index move from a fixed-array field of a mutable local/async-frame aggregate (`s.items[0]`, including nested fields) now resolves the owning field path, clears the element’s flag under the struct drop tree, and emits the MIR move marker. The front end accepts the shape only when the root is a mutable local (not a const binding, parameter, reference/pointer, or temporary); other array-field shapes remain `SAFE-MOVE-001`/`SAFE-MOVE-004`. Added TCC/LLVM runtime coverage for nested array-field moves and negatives for const and reference-root fields. TCC/LLVM negative `831/831`; TCC/LLVM safety `85/85`.
*Revision note v5.43 (2026-11-05): temporary aggregate field-array elements. `make().items[CONST]` can now move an element out of a temporary aggregate’s fixed-array field: the lowered element address recovers the temporary root, the temporary is registered as a block-scope destructible with its field path, and the element flag is cleared. Direct temporary arrays (`make()[CONST]`) remain rejected because array-value returns still have an unrelated backend limitation. Added TCC/LLVM runtime coverage for the temporary field-array shape while retaining the direct-temporary-array rejection. TCC/LLVM negative `832/832`; TCC/LLVM safety `86/86`.
*Revision note v5.44 (2026-11-05): direct temporary array elements. The C emitter now copies an array-valued sret return into its destination with `memcpy` instead of `*(sret) = array`, which C rejects; array-returning functions and `make()[CONST]` work on TCC. The move front end accepts direct temporary-array roots, and the lowering registers the temporary array as a block-scope destructible so the moved element flag is cleared and the remainder drops once. Added TCC/LLVM runtime coverage for `make()[0]` and flipped the old temporary-array negative to positive. TCC/LLVM negative `832/832`; TCC/LLVM safety `87/87`.
*Revision note v5.45 (2026-11-05): nested non-Copy array destructuring. `var [[a, b], [c, d]] = matrix` on an owned mutable local now moves each leaf element, clears its inner-array drop flag through `drop_flag_for_nested_index`, registers each binding for destruction, and leaves wildcard/rest elements owned by the source. Nesting deeper than two fixed-array levels and borrowed/slice views remain `SAFE-MOVE-004`. Added TCC/LLVM runtime drop-count coverage for the nested move and flipped the old nested-non-Copy negative to positive. TCC/LLVM negative `832/832`; TCC/LLVM safety `88/88`.
*Revision note v5.46 (2026-11-05): nested large reference arrays in HRTB. A fixed array of more than 64 direct references now gets a wildcard leaf at any aggregate depth, not only at the top level or one direct struct field. The MIR function-type signature records the full `element -> field -> wildcard` path (including arrays of generic structs), keys the wildcard element loan by the owning root and leaf field index, and the call-site path walk resolves nested field/element chains to the same loan. Type verification propagates the wide-array allowance through array element recursion. Array-of-structs results now keep the caller source loan live; a later mutable borrow of the source rejects with `SAFE-BORROW-001`. Unsized arrays still reject with `SAFE-BORROW-099`, and same-index wildcard leaves under one root conservatively share one loan. Added TCC/LLVM positive compile tests plus alias-conflict negatives for nested structs and arrays of structs. TCC/LLVM negative `836/836`; TCC/LLVM safety `88/88`; main TCC/LLVM `2489/2489`, `2487/2487`; interpret `1938/1938`; async `83/83`; TCC libs `736/736`; `MIRTests` `4749`; audit no growth.
*Revision note v5.47 (2026-11-05): first no-panic effect slice. `func ... : T [no_panic]` is parsed after the return type/where clause; the deprecated postfix array parser looks ahead and leaves an effect-only bracket list for the function signature. The flag is carried into MIR symbol records, and a post-lowering scan rejects a `[no_panic]` body that calls an unannotated callee, calls through a function pointer or dynamic dispatch, throws, or reaches a generated panic call (`__chc_bounds_fail`, overflow/division/shift failures, allocation failure, etc.). Calls to `[no_panic]` callees are accepted; `unreachable` is still an unsafe trap, not a panic. Only the panic effect is implemented; other proposal-0005 effects, module defaults, and async/generic effect propagation remain pending. Added positive/negative `[no_panic]` tests. TCC/LLVM negative `845/845`; TCC/LLVM safety `88/88`; main TCC/LLVM `2489/2489`, `2487/2487`; interpret `1938/1938`; async `83/83`; TCC libs `736/736`; `MIRTests` `4749`; audit no growth.
*Revision note v5.48 (2026-11-05): async executor raw-state boundary. The public executor entry points `exec_make_waker`, `executor_set_wake_hook`, `exec_enqueue_task`, `executor_poll_tasks`, and `executor_wait` accepted raw `*mut Executor`/`*mut TaskVTable`/frame pointers through safe signatures; safe code could pass forged or null state and make the runtime dereference it. They are now `unsafe func`s with null rejection, and queued entries with a null frame/vtable are skipped. `TaskVTable`, `RawTask`, `Executor`, `JoinState`, `SpawnTask`, `SelectState`, `TimeoutOrState`, and `BlockingState` now carry explicit `!Send`/`!Sync` impls so a representation change cannot silently make runtime-owned state transferable. The `window` bridge wraps the raw executor calls in explicit unsafe blocks, and the reactor tests acknowledge the waker boundary. Extended the TSAN smoke to race `RwLock` writers alongside the existing `Mutex` scenario. Negative `848/848`; safety `88/88`; TCC/LLVM main `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; TSAN pass; `MIRTests` `4749`; audit baseline reviewed at `unsafe_blocks 3828`, `unsafe_values 4971`, `unsafe_func_decls 57`, `raw_pointer_types 9416`, `panic_calls 142`.
*Revision note v5.49 (2026-11-05): TSAN breadth for async/concurrency. The `thread_tsan` smoke now also exercises a cross-thread `async::channel` producer/consumer handoff, 200 thread-pool increments through `std::concurrent.create_pool`/`submit_void`, and an executor `spawn_send` future that suspends through `async::yield_now`, alongside the existing `Mutex` and `RwLock` writer races. The executor raw-state hardening from v5.48 remains; `window` and reactor call sites stay explicit unsafe boundaries. No compiler or safe-API change in this slice. TSAN pass; negative `848/848`; safety `88/88`; TCC/LLVM main `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; audit baseline reviewed at `unsafe_blocks 3828`, `unsafe_values 4971`, `unsafe_func_decls 57`, `raw_pointer_derefs 1085`, `raw_pointer_types 9416`, `panic_calls 142`.
*Revision note v5.50 (2026-11-05): nested conditional `Unpin` in generic-impl bounds. `check_generic_impl_bounds` rejected a concrete nested argument such as `Holder<[1]Wrapper<int>>` even when `impl<T> Unpin for Wrapper<T> where T: Unpin` applies: the bound path checked the array element's raw field shape (which contains `PhantomPinned`) and never materialized the conditional positive impl. The `Unpin` branch now retries after recursively asking the normal generic-impl machinery to instantiate conditional positive impls in array elements, generic arguments, and aggregate fields; only concrete candidates are instantiated, so a `Wrapper<PhantomPinned>` argument still rejects with the bound diagnostic. Added positive and negative compile tests for the nested-array case. Negative `850/850`; safety `88/88`; TCC/LLVM main `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.51 (2026-11-05): `'static` borrowed views are transferable. Borrowed `span`/`mut_span`/`str`/`string_view` values still store raw pointer/length pairs, but the parser already puts the source lifetime on the generic type (`&'static [T]` -> `span<'static, T>`, `&'static str` -> `str<'static>`). The auto-trait walk now treats those views as a lifetime-aware structural rule instead of a blanket raw-pointer rejection: only an explicit `'static` lifetime argument makes them `Send`/`Sync` (for `span<T>`, `T: Sync`; for `mut_span<T>`, `T: Send`/`T: Sync`; `str`/`string_view` are immutable and unconditional). `type_satisfies_static_bound` and lambda capture-borrow detection use the same rule, so an elided or named view is still `!Send`/`!Sync` and cannot cross a `'static` boundary. Added positive compile coverage for static `str`/`span`/`mut_span`/`string_view`, negative coverage for bare views satisfying `Send`, and a runtime thread test sending a `&'static str`. Negative `853/853`; safety `89/89`; TCC/LLVM main `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.52 (2026-11-05): arbitrary-depth non-Copy fixed-array destructuring. The array-pattern lowering no longer stops at two fixed-array levels. A new `drop_flag_for_indices` walks the generated drop-field tree through the full constant index path and returns the leaf/element drop reference, so `var [[[a, b], [c, d]], [[e, f], [g, h]]] = cube` moves each leaf and clears only its owning flag. The same resolution now drives nested constant-index assignment (`cube[a][b][c] = ...`), including large inner flag blocks and element templates, and a dynamic final-index write into a fully initialized large array (`arr[i] = value`). Partial dynamic writes into an uninitialized array still reject with `SAFE-INIT-001`. Large/template or borrowed shapes whose flag tree cannot be resolved still reject with `SAFE-MOVE-004` rather than guessing. Added a TCC/LLVM runtime drop-count test for a three-level array of destructor-bearing elements. Safety `91/91`; negative `854/854`; TCC/LLVM main `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.53 (2026-11-05): dynamic-index large-array element moves. `var x = arr[i]` now resolves the runtime element flag (or generated-glue element template) for a fully initialized large array, clears exactly that element's flag, and emits `MoveOut`; the move checker deliberately treats a dynamic index path as the owning root so the root must be fully initialized. Small scalar-flag arrays reject with `SAFE-MOVE-004`, and uninitialized arrays reject with `SAFE-INIT-001`. Added a TCC/LLVM runtime drop-count test (100 zeroed elements, one dynamic move) and a negative uninitialized-root test; the pre-existing small-array dynamic-move negative now carries `SAFE-MOVE-004`. Safety `92/92`; negative `855/855`; TCC/LLVM main `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.54 (2026-11-06): partial dynamic large-array writes. A dynamic final-index write (`arr[i] = value`) may now initialize an otherwise uninitialized large array, including a large array field of an uninitialized struct. The runtime element flag records exactly which element is live; the move checker marks the array partially initialized, so a later constant initialization remains legal while a whole-array read/move or dynamic read/move still requires full static initialization. The lowering's existing runtime-flag requirement is unchanged, so small scalar-flag arrays still reject with `SAFE-MOVE-004`; dynamic moves from partial arrays remain `SAFE-MOVE-001`/`SAFE-INIT-001`. Added a positive compile test, runtime drop-count tests for direct and field arrays, and negatives for partial dynamic read and whole-array move. TCC/LLVM safety `94/94`; negative `857/857`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.55 (2026-11-06): runtime scalar-flag selection for small arrays. Fixed arrays of directly destructible elements now select the element's scalar drop flag at runtime (a bounds-checked pointer table) instead of rejecting every dynamic index. Dynamic writes into initialized or uninitialized small arrays and dynamic moves from fully initialized small arrays now work, including a constant outer prefix (`arr[0][i]`); constant index prefixes also resolve back to the owned array root in the front end. Small arrays whose elements own generated glue still reject with `SAFE-MOVE-004`/`SAFE-INIT-001` because one element owns several scalar leaf flags; dynamic prefixes before the final index and partial dynamic moves remain rejected. Added TCC/LLVM runtime drop-count tests for small-array moves, partial small-array writes, and nested small-array writes/moves, plus positive/negative compile coverage. Safety `98/98`; negative `860/860`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.56 (2026-11-06): checked moves from partially initialized arrays. `MIRMoveState` now distinguishes `PartiallyInitialized` (some children initialized, none moved) from `PartiallyMoved`, so whole-value uses of a partially built aggregate keep `SAFE-INIT-001`. A dynamic move of a directly destructible element from a partially initialized array now passes the move checker and lowering checks the selected runtime flag first, aborting through the new `__chc_uninit_fail` runtime entry if the element is not live. Fully initialized generated-glue/flag-array sources keep their existing static proof; partially initialized generated-glue sources reject with `SAFE-MOVE-001`, and wholly uninitialized arrays still reject with `SAFE-INIT-001`. Added TCC/LLVM runtime tests for dynamic moves after a dynamic write and after a constant partial write, a runtime-abort test for an uninitialized element, and negative coverage for generated-glue partial moves. Safety `101/101`; negative `861/861`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.57 (2026-11-06): generated-glue small-array element rows. A small array of generated-glue elements now resolves a runtime index by selecting the element row and then destroying/arming/clearing every leaf flag in that row. This supports dynamic writes into initialized or uninitialized small arrays, dynamic moves from fully initialized small arrays, constant-prefix inner selection (`arr[0][i]`), and field arrays. Partial dynamic moves from generated-glue arrays remain rejected (`SAFE-MOVE-001`) because the runtime check would have to prove every leaf flag live; direct elements use the single-flag `__chc_uninit_fail` check. Removed the stale `SAFE-MOVE-004`/`SAFE-INIT-001` rejections for generated-glue small arrays and added TCC/LLVM runtime drop-count tests plus positive compile coverage. Safety `103/103`; negative `861/861`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.58 (2026-11-06): two-dimensional runtime indices. A small 2D array of directly destructible elements now supports two runtime indices (`arr[i][j]`) and a constant prefix before the pair (`arr[0][i][j]`). Lowering records the immediate parent's lowered index in `MIRExprResult` and builds a bounds-checked flat pointer table over the N*M scalar element flags (capped at 1024); the same selector is used by dynamic writes and moves, and partial moves keep the `__chc_uninit_fail` check. A runtime outer index with a constant inner index (`arr[i][0]`), generated-glue 2D arrays, and mixed small/large dimensions remain explicit rejections. Added TCC/LLVM runtime tests for 2D dynamic write, move, partial move, and constant-prefix move, plus generated-glue and constant-inner negatives. Safety `107/107`; negative `863/863`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.59 (2026-11-06): runtime outer with constant inner index. A small 2D array of directly destructible elements now moves and writes `arr[i][0]` (runtime outer, constant inner) through the same flat flag selector; the lowerer creates the inner index constant and move checking keeps the owning array path for a runtime-index base instead of projecting a static row. A runtime index before a constant middle index (`arr[i][0][j]`) now rejects instead of clearing an unrelated prefix flag. Added TCC/LLVM runtime write/move/partial-move tests for `arr[i][0]`, positive `arr[i][0]` compile coverage, and a deeper-dynamic negative. Safety `110/110`; negative `864/864`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.60 (2026-11-06): runtime-checked dynamic reads. A scalar field read through a runtime-selected directly destructible array element now checks the selected element flag first (through the 1D/2D/field-array selectors) and marks the derived field address `VF_INIT_CHECKED`; the move checker accepts that guarded load while still rejecting moved/destroyed paths. Dynamic reads of generated-glue elements remain `SAFE-INIT-001` because every leaf would need checking. Non-dynamic field loads keep the original `field_load` provenance, preserving borrow/lifetime checks. Added TCC/LLVM runtime read tests for large, two-index, and field-array partial writes, a runtime-abort test for an uninitialized element, and a generated-glue dynamic-read negative. Safety `114/114`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.61 (2026-11-06): aggregate and generated-glue read guards. Constant-index aggregate/method reads (`arr[1].field.method()`) now resolve every constant index and require all tracked leaves of the selected element to be live, closing the path that bypassed the MIR `Load` check. Dynamic reads of generated-glue elements now check every leaf of the runtime-selected element: small arrays branch over the selected element row, large templates index the leaf flag arrays. The same helper also covers a runtime outer with a constant inner (`arr[i][0].field`). Added TCC/LLVM runtime tests for constant aggregate abort/success, dynamic outer/constant inner abort, and dynamic generated-glue abort/success; the generated-glue dynamic-read negative became a positive compile test. Safety `119/119`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.62 (2026-11-06): mixed-dimension two-index selection. `arr[i][j]` now works when exactly one dimension is large: outer-small/inner-large selects the outer row's flat element flag block, and outer-large/inner-small selects the inner element's outer-indexed flag array. Writes, moves, partial moves, and reads share the selector. Both-large 2D arrays, generated-glue 2D arrays, and a runtime index before a constant middle index remain rejected. Added TCC/LLVM runtime/read tests for both mixed layouts and a mixed partial move. Safety `122/122`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.63 (2026-11-06): both-large two-index arrays. `arr[i][j]` now selects from the outer element template when both dimensions are large: per-inner flag arrays are indexed by the outer element, and an inner dimension above 1024 uses one flat `outer * inner_count + inner` block. Writes, moves, partial moves, and reads share the selector. Generated-glue 2D arrays, a runtime index before a constant middle index, and small/small arrays above 1024 total flags remain rejected. Added TCC/LLVM runtime tests for per-inner, flat-inner, and both-large partial moves. Safety `125/125`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.64 (2026-11-06): small/small generated-glue two-index arrays. `arr[i][j]` on a small 2D generated-glue array now branches the outer and inner element rows for writes, moves, partial moves, and reads; destructive writes arm the selected element's leaves, while moves and reads prove every leaf live first. Outer-small/inner-large generated glue stays rejected because its drop stream falls back to a whole-array flag, and outer-large or both-large generated glue is not represented by the selector yet. Added TCC/LLVM runtime tests for write/read, move, and uninitialized-move abort. Safety `128/128`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.65 (2026-11-06): template-row drop fix and inner-large generated glue. `destroy_drop_fields` now offsets indexed array rows before drilling into their templates, and `destroy_array_drop_fields` dispatches element-template rows through `destroy_array_template`; this fixes dropped elements in outer-small/inner-large nested arrays. With the drop stream correct, small/small and outer-small/inner-large generated-glue 2D arrays support dynamic writes, reads, moves, and partial moves; a move checks every selected leaf first. Outer-large/both-large generated glue remains rejected because its drop tree is not represented. Added TCC/LLVM runtime tests for inner-large write/read, move, and uninitialized-move abort. Safety `131/131`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.66 (2026-11-06): outer-large generated glue and constant two-index paths. Constant `arr[outer][inner]` writes and moves now resolve the full index chain instead of only the final index, including outer-large direct and generated-glue templates. `MIRDropElementRef.template_flag_index_const` carries the outer element index for templates whose leaf flag arrays are indexed by the outer element. Outer-large/inner-small generated glue now supports dynamic writes, reads, moves, and partial moves through an inner-row branch; uninitialized moves check every leaf first. Both-large generated glue remains rejected. Added TCC/LLVM runtime tests for constant move, dynamic write/read, dynamic move, and uninitialized-move abort. Safety `135/135`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.67 (2026-11-06): both-large generated glue. The nested generated-array drop tree now carries `inner_count` for outer-large templates, and a both-large template with inner > 1024 stores leaf flag arrays as one flat `outer * inner_count + inner` block. Two-index selection, constant writes/moves, dynamic writes/reads/moves, and uninitialized move checks now cover `[100][100]S` and `[100][2000]S`. Added TCC/LLVM runtime tests for constant move, dynamic write/read, dynamic move, flat-inner access, and uninitialized-move abort. Safety `140/140`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.68 (2026-11-06): dynamic/constant/dynamic three-index access. `arr[i][CONST][j]` now works for small directly destructible 3D arrays: writes, reads, moves, and partial moves select the middle row at compile time, branch the runtime outer row, and select the runtime inner flag. The lowering records the outer index of a nested aggregate (`MIRExprResult.base_outer_index_value`) and adds `drop_flag_for_dynamic_constant_dynamic`. Moves/reads prove the selected flag live first. Added TCC/LLVM runtime tests for write/read, move, and uninitialized-move abort. Safety `143/143`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.69 (2026-11-06): three runtime indices. `arr[i][j][k]` now works for small directly destructible 3D arrays: writes, reads, moves, and partial moves branch the runtime outer and middle rows, then select the runtime inner scalar flag. The lowering detects the three-runtime form from the grandparent index, and moves/reads prove the selected flag live first. Added TCC/LLVM runtime tests for write/read, move, and uninitialized-move abort. Safety `146/146`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.70 (2026-11-06): runtime-tree move/drop closure. Three gaps are closed. (1) Small/small direct 2D arrays now select every shape up to the 64 x 64 scalar-flag maximum (4096 flags); the old 1024-flag cap is gone. (2) Generated-glue 3D arrays with expanded dimensions now support runtime writes, reads, moves, partial moves, and uninitialized-move aborts, including mixed constant/runtime chains such as `arr[i][0][j]` and `arr[0][i][j]`. (3) Expanded small arrays support arbitrary runtime depth: 4D/5D (`arr[i][j][k][l]`) writes, reads, moves, and uninitialized-move aborts work through the same recursive runtime-tree selector. The selector collects the full `IndexAddr` chain from MIR rather than relying on two fixed base indices. Added TCC/LLVM runtime tests for 4D read/write/move and abort, generated 3D read/write/move and abort, generated 3D mixed constants, 64 x 64 direct and generated 2D arrays. Residual 3D gap: a generated-glue array with a large middle or inner dimension still rejects because that level is compressed. Safety `153/153`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.71 (2026-11-06/07): compressed 3D generated-glue levels. Generated-glue 3D arrays now select a runtime element when the inner, middle, or outer dimension is large. The new selectors cover inner-large templates, middle-large templates with expanded inner rows or a flat inner block, and outer-large templates with expanded/flat inner rows. Writes, reads, moves, partial moves, mixed constant/runtime chains, and uninitialized-move aborts work for `[1][2][100]S`, `[1][100][2]S`, `[1][100][100]S`, `[65][2][2]S`, `[65][2][100]S`, `[65][100][2]S`, and `[1][2][2000]S`. The pre-existing one-flag-per-element flat representation for a generated inner array above 1024 nested under a large outer/middle template remains reject-only. Added TCC/LLVM runtime tests for inner/middle/both-large/outer-large/flat-inner writes, reads, and moves, plus a compressed uninitialized-move abort. Safety `159/159`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.
*Revision note v5.72 (2026-11-07): generated flat inner arrays above 1024. A generated-glue inner array larger than 1024 under a large outer/middle template now builds one flat flag block per direct leaf field instead of one flag per element. Scope destruction loops the flat inner dimension, offsets each element address by the recorded element type, resolves the direct leaf field, and drops exactly the live leaves. The 2D both-large selector recognizes the multi-leaf flat template, so 2D and compressed 3D writes, reads, moves, partial moves, and uninitialized-move aborts work for `[65][1200]S`, `[1][65][1200]S`, and `[65][1][1200]S`. Generated elements with nested destructible leaf paths still use the legacy one-flag-per-element representation and remain reject-only for per-field selection. Added TCC/LLVM runtime tests for both-large 2D, middle-large 3D, outer-large 3D, and an outer-large uninitialized-move abort. Safety `163/163`; negative `865/865`; main TCC/LLVM `2489/2489`, `2487/2487`; async `83/83`; TCC libs `736/736`; plugins `1305/1305`; interpret `1938/1938`; `MIRTests` `4749`; TSAN pass; audit no growth.