---
name: Safety Implementation
description: How the Chemical safety system is implemented and tested — the full-parity plan, stable SAFE-* diagnostics, report→migrate→enforce workflow, safety test suites, and current implementation status. Load before implementing or reviewing any safety rule, borrow-checker change, or unsafe-boundary check.
---

# Chemical Safety Implementation

This skill is the working guide for the full safety parity program. The canonical
plan is [`lang/docs/safety-implementation-plan.md`](../../../lang/docs/safety-implementation-plan.md).
The source tree wins over any status note here.

## Current status (2026-10-27)

**M0 (S0.1–S0.6) is implemented. M1 has started.**

Implemented:

- `Diag` has an optional stable `code` (`core/diag/Diagnostic.h`).
- Coded diagnostics render as `error[SAFE-*]:` / `warn[SAFE-*]:`; uncoded
  diagnostics keep the old `error:` rendering.
- `Diag::with_code(...)` and `Diag::format`/`Diag::ansi` include the code and
  related-information notes.
- `ASTDiagnoser`/`Diagnoser` can attach codes through
  `empty_diagnostic(..., code)` and `with_code`.
- First real codes:
  - `SAFE-INIT-001` — use of an uninitialized variable.
  - `SAFE-UNSAFE-001` — dereference of a raw pointer in safe context
    (report-only warning today).
  - `SAFE-UNSAFE-002` — call to an `unsafe func` outside an `unsafe` context
    (error; covers expression calls, methods, and unsafe constructors).
- Negative harness accepts both `error:` and `error[SAFE-` and has
  `expect_compile_warning_id`.
- Safety suite at `lang/tests/safety/` (`--safety`), wired through
  `lang/tests/build.lab` and `scripts/test.sh`.
- Runtime-failure harness (`expect_runtime_panic`) using `process::execute` and
  compiler subprocesses; smoke + panic tests pass on TCC and LLVM.
- `--safety-report` read-only audit (S0.2): text table and JSONL
  (`--safety-report json`), collected after type checking and before codegen.
  Categories include raw pointers, unsafe blocks, FFI, panics, allocators,
  memory calls, inline asm, and mutable globals. Per-module dedup. Generic
  master impl bodies are skipped for now; concrete declarations are reported.
- Tests: `lang/tests/negative/src/safety_diagnostics.ch`; safety suite now has
  smoke, report, and runtime-panic tests (`3/3` on TCC and LLVM).

- Module/CLI safety opt-outs removed (S0.4): `safety`, `safe_mode`, and
  `checks.*` no longer exist; `option safety = false` is rejected. Module-file
  `option` parsing now consumes its leading identifier.
- `unsafe(expr)`, `da_in_unsafe`, and `@allow_zeroed` are still kept until their
  replacement stages; they are not user-facing opt-outs.
- S0.5 MIR/interpreter hardening (partial): O(log n) source interning, hard
  errors for unknown interpreter opcodes, `AtomicFence` no-op for single-threaded
  comptime, and `--mirtests` (`4524 checks`). Interpreter provenance/bounds/UAF
  checks are deferred to M5.7.

- S0.6 `--check` runs MIR lowering + structural verification without emission
  or linking; a good module exits 0 with no executable, an uninitialized read
  exits 1 with `SAFE-INIT-001` and no executable.

- S3.1a move-path trees: `MIRBuilder::new_place` attaches a
  `MIRMovePathNode` tree to every owned place (a destructor-bearing aggregate
  or an array of one), with one node per owned field and fixed-array element.
  All links use `MIR_INVALID_ID` for "none", so path 0 is valid; the builder
  propagates initialized/moved/destroyed state through the tree on
  init/copy-init/move-init/assign/drop/destroy, and `MIRVerifier` checks the
  tree structure. `MIRTests` covers a struct + `[3]Destructor` tree and move
  propagation.
- S3.1b/S3.2 MIR init/move dataflow: `compiler/mir/MIRMoveCheck.{h,cpp}` runs
  over every lowered function (normal and `--check`) before either backend.
  It propagates Uninitialized/Initialized/Moved/Destroyed through the CFG and
  reports `SAFE-INIT-001` / `SAFE-MOVE-001` for moves, drops, destroys and
  reads of dead or conditionally initialized paths. Disagreeing joins become
  `Maybe`; reads/moves/unconditional drops reject it, while a conditional
  `Drop` is accepted and its constant `set_drop` flag is resolved (false is a
  no-op, true requires initialization). Parameters/globals start initialized
  and coroutine frame fields are skipped (owned by the async lowering).
  `unsafe(...)` is not an opt-out: the negative suite proves an uninitialized
  move inside `unsafe` is still rejected, and conditional-init use is rejected.
  `MIRTests` is `4563 checks`.
- S3.2b array index moves: `var x = arr[i]` (and `arr[0]`) is rejected with
  `SAFE-MOVE-001` when the element owns a destructor; the old lowering copied
  the element out while the array still dropped it (double free). Whole-array
  moves, `arr[i].field` reads, and `Copy` elements are still allowed.
- S3.1c field-wise initialization state: the "writing one member initializes
  the whole struct" heuristic is gone. A projection store resolves its exact
  move-path node: a write into a moved/destroyed value is `SAFE-MOVE-001`, and
  a chained read (`s.b.id`) is checked through the derived `field_addr` path.
  Construction temporaries built field by field promote their aggregate to
  initialized when every owned child is written, so the un-zeroed-construction
  optimization stays valid. Partial moves demote ancestors to
  `PartiallyMoved`, so a whole-value drop/move of a partially moved aggregate
  is rejected.
- S3.1d field-wise drop elaboration: a bare local of a struct with generated
  drop glue (`var s : S`) gives every owned field its own drop flag, including
  one level of grandchildren (`s.inner.x`); a fixed array of destructible
  elements (<= 64) gets one flag per element. A write destroys the old value
  only when the target is live, scope exit/reassignment destroy only
  initialized entries (array elements last-to-first), and moving the aggregate
  clears every flag. A user `@delete`, a large array, a dynamic index and
  projections deeper than one extra level still require the destination to be
  initialized. A mutable raw escape (`&raw mut x`, `&raw mut x[i]`) switches
  the place to the whole-value drop and makes the dataflow trust it (the
  `Option::take` / `archive::extract_entry` pattern). A place-to-place `store`
  requires a live source; `memcpy`/`memmove`/`memset` (direct or sret)
  initialize their destination. Unions are raw storage:
  `type_needs_move_path` does not recurse into union fields. A variant is one
  owned value (its tag-aware generated destructor owns the active payload), so
  it gets a root path with no payload children; payload moves remain a
  front-end pattern-match effect. `MIRTests` is `4585 checks`.
- S3.4 dropck report: `SAFE-DROP-002` warns when a user `@delete` is declared
  on a type with lifetime parameters or a reference-typed field (the type may
  observe borrowed data at drop time). Enforcement waits for regions/M4; the
  warning is report-only and deduplicated per declaration location.
- S3.6 single init/move source: the flow-sensitive AST definite-assignment
  pass (`locals`/`init_bits`/`scope_stack`, `da_in_unsafe`, `is_first_init`)
  and the AST move tracker (`mark_moved_value`/`check_id`/`check_chain`,
  `moved_identifiers`/`moved_chains`, `is_moved` flags) are removed.
  `MIRMoveCheck` is the sole init/move/drop source. The lowering emits a
  `MoveOut` marker for by-value call arguments so a moved/uninitialized
  argument is checked, clears a pattern payload's source variant, and a
  `TypeVerify` check rejects moving a destructor-bearing element out of an
  array by index (`SAFE-MOVE-001`). Module-scope uninitialized globals are
  still diagnosed in `TypeVerify`. First assignment destroys the old value
  through the runtime drop flag instead of `is_first_init`.
- M4 started, S4.2a region signatures: `compiler/mir/MIRRegionSignature.{h,cpp}`
  defines `MIRRegionSignature` (declared lifetime count, per-parameter region,
  return region, `'a: 'b` outlives, HRTB binders reserved) and the pure
  `make_region_signature` mapping (`static`/`elided`/`self`, quoted or bare
  spellings). `MIRModule::region_signatures` is an append-only side table;
  `MIRFunction::region_signature` links a function to its record. The lowerer
  builds the signature from the declaration before body lowering, and
  `MIRDump` prints it (`; regions:`). `MIRTests` covers the mapping, module
  table and dump (`4603 checks`).
- S4.3a borrow opcodes: `Borrow`/`BorrowMut` are explicit MIR operations
  (`Value(addr), Constant(region)`, result is the reference-typed pointer).
  `ReferenceOfValue` lowering (`&x`, `&mut x`, `&*p`, `&obj.field`,
  `&arr[i]`) wraps the computed address in the matching borrow; `&raw` stays
  `AddressOf`. Emitters pass the address through (C/LLVM), the interpreter
  copies it, and the dataflow treats it as a read. The region operand is a
  placeholder (`MIR_INVALID_ID`) until NLL assigns region variables (S4.4).
  `MIRTests` covers the opcodes, dump and C emission (`4610 checks`).
- S4.4a loan analysis: `compiler/mir/MIRBorrowCheck.{h,cpp}` tracks
  `Borrow`/`BorrowMut` loans, follows references through reference locals
  (Store/MoveInit/CopyInit/Load/Copy/Cast), and reports overlapping live loans
  (`SAFE-BORROW-001`) and moves/drops of a borrowed place (`SAFE-BORROW-002`).
  Both are now enforced (hard errors) after the in-tree migration reached zero
  hits; `ASTProcessor` prints enforced diagnostics as `error[SAFE-*]`.
- S4.4b CFG liveness + region variables: the loan analysis now computes
  per-block `live_in`/`live_out` over loans (worklist over the MIR CFG) and
  walks each block backwards to check conflicts/moves at the exact
  instruction, so loans in disjoint branches do not conflict. Reading a
  reference local counts as a use even when the value is discarded. Each
  `Borrow`/`BorrowMut` gets a fresh function-local region id
  (`MIRFunction::region_var_count`, seeded from the signature's declared
  lifetime count) instead of the placeholder. `MIRTests` is `4617 checks`; the
  negative suite has alias/sequential/shared/move/branch cases.
- S4.5a callee signatures: region signatures are stored per symbol at interning
  time, so a caller sees a callee's signature before its body. A known callee
  that returns a reference keeps the single reference argument's loan alive
  through the call result; a non-reference-returning callee ends the loan at
  the call. Indirect/extern callees and ambiguous multi-reference results keep
  the conservative escape mark. Multiple reference arguments are mapped by the
  return region (identity first, then a unique outlives source).
- S4.5b reference escape on return: `SAFE-LIFE-001` reports returning a
  reference to a local, temporary or coroutine frame field (storage that dies
  at function exit). A returned parameter/global reference is allowed. It
  found a genuine dangling reference in the test suite
  (`get_destruct_or_not` returned `&d` to a local); the dead function was
  removed and **SAFE-LIFE-001 is now enforced** (a hard error). `MIRTests` is
  `4619 checks`; the negative suite has local/parameter/ref-return cases
  (450 tests).
- S3/M3 refinement (found by the server/tls suite): a local of a struct with a
  user `@delete` may be built field by field (the hand-written constructor
  pattern) as long as it is moved/returned as a whole; direct-child writes are
  tracked in the dataflow, the whole-value drop flag stays disarmed, and a
  partial value that is not moved leaks (safe). A partial move/return is still
  `SAFE-INIT-001`/`SAFE-MOVE-001`. Also fixed a double drop on
  `*ptr = owned_local` (the dereference-assignment path now marks the source
  moved), which the tls module exposed.
- S4.4c storage lifetimes: the lowering emits `StorageLive`/`StorageDead`
  markers for every local in each lexical scope (no runtime effect). The loan
  analysis reports `SAFE-LIFE-002` when a loan is still live at the
  `StorageDead` of the local it refers to. References stored in struct fields
  are followed through field stores and whole-value moves, so
  `h = H { r: &x }` used after `x`'s block is detected. `MIRTests` is `4620
  checks`; negative tests cover the struct-field escape and the same-scope
  positive (452 tests). **SAFE-LIFE-002 is enforced.** The first run over the
  universal suite exposed 40 false positives from loop back-edges: `block_use`
  was a plain use set, so a loan defined and used in the same loop block was
  treated as live-in. The gen/kill computation now only counts uses that occur
  before the block's definition; the universal suite is 368/368 with zero
  borrow warnings.
- S4.6a return-region outlives: `SAFE-LIFE-003` (enforced) checks that a
  returned reference sourced from a parameter outlives the declared return
  region: identity, `'static`, or a transitive `where 'a: 'b` edge. Parameter
  places carry `MIRPlaceDef::param_index`, and reference values/places track
  their parameter index through Load/Store/Copy/Cast/MoveInit, so returning a
  parameter directly or via a local is covered. The borrow pass no longer has
  a "no Borrow instruction" fast path (a pure `return x` still needs the
  signature check). `MIRTests` is `4623 checks`; negative tests cover missing
  and satisfied outlives bounds (454 tests).
- S4.6a aggregate returns: references carried in fields of a returned struct are
  checked too. Field stores record both `field_loans` and `field_params`; whole
  aggregate moves transfer them; a returned reference field is loaded with its
  parameter index. Aggregate returns use the hidden sret pointer (the value is
  stored to the first entry parameter, then `return` has no operand), so the
  borrow pass recovers the returned source place. Region signatures now read a
  generic type's first lifetime argument (`Holder<'a>`), and a cached signature
  built before the return type was linked is rebuilt in `lower_function`.
  `MIRTests` is `4630 checks`; negative tests cover aggregate-return region,
  local-field escape, satisfied bound, and direct field return (458 tests).
- S4.5 multi-reference call mapping: a reference-returning callee with several
  reference arguments now maps its result to the argument whose declared region
  is the return region (identity first, then a unique `where 'a: 'b` source).
  The other argument loans end at the call, so `pick(&mut a, &mut b)` no longer
  loses the result loan to the conservative escape path. Ambiguous results
  still escape. Lambdas carry their enclosing function symbol; borrow
  diagnostics print `inside <parent>` and the source-location provider is
  installed even in `debug_quick`, so a generated lambda warning points at the
  enclosing function. Migrated the one in-tree two-`&mut` test; the tree now
  has zero `SAFE-BORROW-001` hits. `MIRTests` is `4631 checks`; negative tests
  cover both the aliased and unrelated-argument cases (460 tests).
- S4.9/S4.10 enforcement: after the tree reached zero hits across every suite,
  `SAFE-BORROW-001` and `SAFE-BORROW-002` were flipped to hard errors. The
  enforcement gate is the same fatal path as the LIFE rules; enforced
  diagnostics render as `error[SAFE-*]`. The negative suite asserts the error
  form for alias, shared/mutable alias, move-while-borrowed, single- and
  multi-reference result loans (`460` tests).
  Next in M4: S4.2b variance, then dropck/async/Pin and provenance.
- S2.6d unsafe invariant constructors: `vector::set_len` and
  `default_function_instance::make2` are now `unsafe func`; the audited
  `fs::read_file` and `std::function` `make` paths wrap their calls in
  `unsafe`. Safe-context calls carry `SAFE-UNSAFE-002`, and the negative suite
  asserts the code for expression calls, methods, and both constructors.

- M1 started: `&'a T` / `&'a mut T` reference lifetimes are parsed and carried
  on `ReferenceType` (S1.1a). `func <'a, T>`, `struct Foo<'a>`, and
  `impl<'a, T>` collect lifetime parameters; duplicate names are rejected
  (S1.1b). Generic lifetime arguments (`View<'a>`) parse in type position and
  are stored on `GenericType::lifetime_args` (S1.1c). Basic lifetime-name
  resolution rejects unknown names while accepting `'static`/`'self`
  (S1.1d). Signature elision ties `&self` outputs to `'self`, one input
  lifetime to the output, and rejects ambiguous multi-input outputs (S1.1e).
  `where 'a: 'b` and `where T: 'static`/`T: 'a` bounds parse and are
  validated for declared names; `&'static T` is accepted (S1.1f/S1.1g).
  Lifetime-only and mixed generic impl targets (`impl<'a> Trait for View<'a>`,
  `impl<'a, T> Trait for View<'a, T>`) and explicit `View<'a> { ... }` /
  `View<'a, T> { ... }` literals work end-to-end (S1.0b); impl-header
  `where` clauses (`impl<T> Copy for Wrapper<T> where T: Copy`, S1.0c) are
  parsed, linked and enforced at instantiation. `std::PhantomData<T>`
  is a zero-sized marker usable as `PhantomData<&'a T>` (S1.3a); variance
  analysis waits for regions. `Copy` is opt-in for user types (S1.4a):
  destructor-bearing types and trivially copyable types without an
  `impl Copy` are rejected with `SAFE-COPY-001`. Enforcement in the borrow
  checker is still pending. Chemical syntax is `func <'a> f(...)`.

Next:

- S1.2 `'static`/bounds and S1.3a `PhantomData` are done; temporary lifetime
  extension and variance analysis move with the borrow checker (M4).
  S1.4a opt-in `Copy` is enforced (`SAFE-COPY-001`) and the in-tree tree is
  migrated. S1.4b validates `impl Copy`: destructor targets are
  `SAFE-COPY-002`, non-Copy fields are `SAFE-COPY-003`; generic fields must be
  promised `T: Copy` via inline or impl-header where clauses (S1.0c). S1.4c
  validates `@delete` signatures (`SAFE-DROP-001`: one `&self`/`&mut self`,
  void return). `Default` bounds are satisfied by primitives or a zero-argument
  constructor (S1.4d). `Clone` bounds are satisfied by an explicit impl or by
  `Copy` (S1.4e); automatic Clone derivation is still pending. `Send`/`Sync`
  bounds are derived structurally from fields (S1.5a); raw pointers are
  neither, references follow the usual rules, and `PhantomData` participates
  through its arguments. Generic-function bounds are now checked for both
  explicit `f<T>(...)` and inferred `f(x)` calls (S1.5b). Explicit opt-ins use
  `unsafe impl Send/Sync`; a safe `impl Send/Sync` is `SAFE-AUTO-001`
  (S1.5c). Negative impls (`impl !Send for T`, concrete targets only) override
  the structural result and conflicts are `SAFE-AUTO-002` (S1.5d). `Clone` is
  implemented for primitives, `string` (deep), `string_view` and `span<T>`
  (S1.4f). Generic-interface method dispatch now resolves `T: Clone` /
  `T: Show` calls through the bound interface and relinks to the concrete impl
  after monomorphization (S1.5e). Bound methods on generic fields
  (`self.value.show()` / `self.value.clone()`) resolve through a target-param
  to impl-param alias while linking the impl body (S1.5f). Speculative generic
  impl discovery is silent on unsatisfied bounds, and impl arguments are
  checked exactly without pointer stripping (S1.5g). Generic variant/union
  field dispatch works through pattern variables (`value.clone()` in a
  `CloneBox<T>` impl, S1.5h). Nested generic-impl instantiations whose arguments
  still contain a generic parameter are signature-finalized but their bodies
  are deferred until the parameter is concrete (S1.5k). `Option<T>`/
  `vector<T>` `Clone` now use inline `<T : Clone>` bounds (the bound must be
  visible while the master body is linked); `Option` reads the payload through
  a `&raw mut value as *mut T` pointer (no shallow-copy temporary or
  `intrinsics::forget`), and `vector` uses a pointer + `memcpy` helper (S1.4h).
  Tests cover deep string copies and exactly-once drops.
  Vector bounds checks were attempted and reverted: they expose an existing
  router-plugin `get_ptr(0)` on an empty vector during module compilation, so
  that OOB must be fixed before checks can be enabled. Structural auto-trait derivation also honors explicit
  positive impls on nested fields (S1.5l), after checking negative overrides. `vector::get_ref` now returns `&T` and `vector::get_mut`
  is the mutable accessor (S1.10a), so mutation through a shared reference is
  rejected. `impl<T> !Send/!Sync for Wrapper<T>` works for all
  instantiations (S1.5i);
  conditional forms with parameter bounds or a where clause
  (`impl<T: Clone> !Send`, `impl<T> !Sync for Wrapper<T> where T: Clone`) are
  evaluated against the concrete arguments at the use site and apply only when
  the bounds hold. `std::UnsafeCell<T>` / `std::Cell<T>` (S1.6a) provide
  interior mutability: `UnsafeCell` is `!Sync` and `Send` follows `T: Send`;
  `Cell` wraps it with `T: Copy` reads. Structural auto-trait derivation now
  consults the implementation index for unconditional generic negative impls
  through nested fields and honors explicit positive impls on nested fields
  (S1.5l). `std::NonNull<T>` (S1.6b) wraps a non-null raw pointer and is
  deliberately neither `Send` nor `Sync`. `std::MaybeUninit<T>` and
  `std::ManuallyDrop<T>` (S1.6c) use the `@no_drop` container flag, which
  suppresses generated and explicit destructors, so their raw storage can be
  written and later moved out with `memcpy`/`intrinsics::forget`.
  `std::Box<T>` (S1.8) is a unique heap owner and derives `Send`/`Sync` from
  `T` through conditional positive impls (S1.5m), which the type checker now
  evaluates against concrete arguments. `std::Rc<T>` (S1.8b) adds
  single-threaded reference counting with unique-owner `get_mut`/`try_unwrap`,
  `Weak<T>` (`downgrade`/`upgrade`), and is structurally `!Send`/`!Sync`;
  the shared value lives in `ManuallyDrop`, and
  `ManuallyDrop::destroy`/`MaybeUninit::assume_init_drop` destroy in place.
  `std::RefCell<T>` (S1.6d) adds runtime borrow tracking through
  `try_borrow`/`try_borrow_mut` guards (`Ref`/`RefMut`) that release on drop;
  it is `!Sync` and `Send` follows `T`. `std::Mutex<T>` (S1.6e) adds
  OS-mutex-backed `lock`/`try_lock` with an unlock-on-drop `MutexGuard`, and is
  `Send`/`Sync` when `T: Send`. `std::RwLock<T>` (S1.6f) adds shared-reader /
  exclusive-writer guards (`pthread_rwlock_t` on POSIX, `SRWLOCK` on Windows);
  it is `Send` when `T: Send` and `Sync` when `T: Send + Sync`, and guards
  release on drop. `std::Arc<T>`/`ArcWeak<T>` (S1.8c) add
  thread-safe shared ownership with mutex-protected counts (atomic swap later),
  unique-owner `get_mut`/`try_unwrap`, and are `Send`/`Sync` when
  `T: Send + Sync`. Static constant indices into fixed arrays are checked at
  type verification (`SAFE-BOUNDS-001`, S5.4a) on reads and writes. Vector
  runtime bounds (S5.4b) reject OOB and empty access in `get`/`get_ptr`/
  `get_ref`/`get_mut`/`set`/`remove`/`remove_last`/`take_last`/`last_ptr` with a
  panic; the safety suite covers OOB, empty pop, and empty last-pointer cases.
  `string`/`string_view`/`u16_string` `get`/`set` (S5.4d) likewise reject
  `index >= size()`; `string_view::subview`/`skip` reject
  `start > end`, `end > size`, and `count > size`. Tests cover string
  read/write and view reads, subviews, and skips. `span::get` rejects
  `loc >= size()` (S5.4e); `deque::get_ptr` rejects `index >= size()` and its
  block growth checks capacity/allocation (S5.4f). `vector` and
  `string` capacity/length growth is overflow-checked (S5.3a), including the
  string NUL byte and capacity doubling. `new T` and `std::box` reject a null
  `malloc` result instead of returning an owner that later writes through null
  (S5.3b). `MaybeUninit::assume_init`/`assume_init_drop` and
  `ManuallyDrop::take`/`destroy` are `unsafe func` (S2.6b), so uninitialized
  reads and manual ownership transfer require an explicit unsafe block.
  `UnsafeCell::get_ref` is `unsafe func` (S2.6c); tracked `RefCell` borrows
  wrap it internally. `Box` panics on `get`/`get_mut`/`into_inner` after
  `into_inner` emptied it (S1.8d); `Rc`/`Arc` and their weak handles panic on
  use after `try_unwrap`/weak release (S1.8e), and `get_mut` is blocked while
  either another strong owner or a weak handle exists (S1.8f).
  `deque::get_ref` returns `&T`
  and `get_mut` requires `&mut self` (S1.10b), matching `vector`. `Option::take` requires `&mut self` (S1.9a), so it cannot rewrite a
  shared/immutable binding. `unordered_map`/`ordered_map` check bucket-growth arithmetic and
  table/node allocations, and `ordered_map` seeds its first table at 16
  buckets (S5.3c). Self-aliasing `string` append records and re-bases the
  source offset across growth (S5.3d), and `string::resize` zero-fills newly
  exposed bytes (S5.3e). Capturing-function heap environments also reject a
  null allocation (S5.3f); `vector::make_with_capacity` and thread-pool
  promise/pool allocations are checked as well (S5.3g). `u16_string` heap,
  append, and substring buffers check `(capacity * 2) + 2` and null
  allocations (S5.3h), and `string::substring` validates its range and heap
  allocation (S5.3i). `u16string::substr` clamps with subtraction and builds
  its result directly (S5.3j). Hash-map node deletion/replacement runs the
  generated node destructor so key/value drops are not skipped (S5.3k).
  `std::mem.replace`/`swap` move values without re-initializing memory and
  `std::mem.forget` suppresses a destructor; the legacy `std::replace`/`swap`
  names delegate to them (S3.0a). `string::erase` clamps with subtraction so
  a huge length cannot wrap and move past the end (S5.3l). Long constant
  `u16string` views move through heap storage on `pop_back`/`set`/
  `mutable_data`, and u16 append length arithmetic is overflow-checked
  (S5.3m).
  Dynamic fixed-array indices (S5.4c) are guarded in MIR lowering with
  `index < length` (zero-extended to u64, so negative indices fail) on reads and
  stores; failure calls `abort` and then `Unreachable`. Pointer/vector/operator
  indexing is excluded because it has its own checks. Integer `/`/`%` guard
  zero and signed `MIN / -1`; `<<`/`>>` guard negative or `>= bit width`
  shift amounts (S5.5a), including compound `/=`, `%=`, `<<=`, `>>=` on
  locals/globals/fields/derefs/elements. Debug `+`/`-`/`*` overflow and unary
  negation remain pending.
  `Box::into_inner` frees storage without re-running the destructor. `unsafe func` (S2.6a) is parsed at top level and
  as a member, and calls from safe code are rejected; `unsafe(expr)`/`unsafe {}`
  are both unsafe call contexts. Partial initialization and partial moves are
  modeled (S3.1c), including construction temporaries and use-after-move
  through assignment. M3 is complete: the next milestone is M4 (regions, NLL,
  borrow checking, dropck enforcement, and async/Pin). The current M4 step is
  S4.5b (enforced local-reference return escape), then S4.2b variance and
  outlives/region constraints.
- Parity corpus/manifest is M8.1 (not an M0 blocker).
- Baseline/performance gate and `scripts/safety-audit.sh` (S0.3).
- Remove module/CLI opt-outs (S0.4).

## Non-negotiable rules

1. **No user opt-out.** No `SafeMode::Off`, module flag, CLI escape, or
   per-dependency exemption.
2. **No error-suppressing allowlist.** A rule is report-only until the whole
   in-repo tree is migrated, then enforced. Report-only is a debug development
   mode and must be compiled out of release builds.
3. **`unsafe` is explicit.** It permits only inherently unchecked operations;
   it never disables ownership, initialization, or borrow checking.
4. **Never accept a pattern the checker cannot prove safe.** Emit a specific
   `SAFE-*` diagnostic and a rewrite suggestion.
5. **Every safety rule has a negative test, a positive test, and (where
   applicable) a runtime-failure test.**
6. **No diagnostic mentions the reference language.**
7. **Every commit is green.** Use the path→suite gate in the plan (§12.7).

## Report → migrate → enforce

Every safety rule follows the same three-step workflow:

1. **Report-only**
   - Add the diagnostic with a stable `SAFE-*` code.
   - Emit it as a warning; behavior is unchanged.
   - Add a negative/positive test and update the audit report.
2. **Migrate**
   - Fix every in-repo occurrence (libraries, tests, `build.lab`, CBI).
   - Report hits must reach zero.
   - Do not add an allowlist.
3. **Enforce**
   - Flip the diagnostic to an error.
   - Include the migration diff and a CI assertion of zero report hits.
   - Add the release-gate negative test.

Runtime rules (bounds, overflow, ABI, drop order) use
**observe/count → migrate → enable** because they have no warning form.

## Adding a coded diagnostic

```cpp
// Simple: code attached at the diagnoser
diagnoser.error(loc).with_code("SAFE-INIT-001") << "message";

// Warning (report-only)
diagnoser.warn(node).with_code("SAFE-UNSAFE-001") << "message";

// If you already hold a Diag&
auto& d = diagnoser.error(node);
d.with_code("SAFE-BORROW-002");
d << "cannot borrow ...";
```

Codes are stable and grouped by domain:

| Prefix | Domain |
|---|---|
| `SAFE-OWN-*` | moves, drops, partial moves |
| `SAFE-BORROW-001` | overlapping live loans (at least one mutable) |
| `SAFE-BORROW-002` | move/drop of a place while a borrow of it is live |
| `SAFE-BORROW-*` | aliasing / loans |
| `SAFE-COPY-001` | user type used with a `Copy` bound but no explicit `impl Copy` |
| `SAFE-COPY-002` | `impl Copy` on a type with a destructor |
| `SAFE-COPY-003` | `impl Copy` on a type whose field is not Copy |
| `SAFE-DROP-001` | invalid destructor signature (value self, missing self, non-void return, async) |
| `SAFE-DROP-002` | report-only: destructor on a type that can hold borrowed data (dropck waits for regions) |
| `SAFE-AUTO-001` | `Send`/`Sync` implemented without `unsafe impl` |
| `SAFE-AUTO-002` | invalid/conflicting negative impl (non-auto trait, duplicate, positive+negative, generic target) |
| `SAFE-MOVE-001` | move of an already-moved value / access or assignment after a move |
| `SAFE-LIFE-001` | returning a reference to a local/temporary/frame field |
| `SAFE-LIFE-002` | borrow still live when the referent's local storage ends |
| `SAFE-LIFE-003` | returned reference does not outlive the declared return lifetime |
| `SAFE-LIFE-*` | lifetimes / escapes |
| `SAFE-INIT-*` | initialization / definite assignment |
| `SAFE-VALID-*` | invalid values / transmute / packed |
| `SAFE-BOUNDS-*` | indexing |
| `SAFE-OVERFLOW-*` | arithmetic |
| `SAFE-SEND-*` | Send/Sync / races |
| `SAFE-UNSAFE-*` | unsafe boundary |
| `SAFE-FFI-*` | FFI / C ABI |
| `SAFE-ASYNC-*` | async/Pin |
| `SAFE-DEP-*` | future remote dependency pass |

Do not reuse a code for a different rule. Do not renumber existing codes.

## Test harness

Compile-fail tests live in `lang/tests/negative/src/`:

```chemical
@test
func neg_example(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "... unsafe source ..."
    expect_compile_error(env, "example", ch, "SAFE-OWN-001")
}
```

Helpers in `lang/tests/negative/src/main.ch`:

- `expect_compile_success(env, name, ch)`
- `expect_compile_error(env, name, ch, expected_substring)`
- `expect_compile_error_with_mod(env, name, ch, expected_substring, mod_content)`
- `expect_compile_warning_id(env, name, ch, expected_id)` — compiles and checks
  `warn[SAFE-*]`.
- `expect_compile_output_contains(...)`
- `expect_emitted_symbol(...)`

Coded errors render `error[SAFE-*]:`; the harness accepts both coded and
uncoded errors. Prefer asserting the **code**, not the full message.

Runtime/parity tests live in `lang/tests/safety/` and run with `--safety`.
Compile-fail tests stay in `--negative` so the default suites remain fast.

## Commands

```bash
# Fast compile-fail gate
./scripts/test.sh --tcc --negative --no-build

# Safety suite (smoke, report, runtime-panic)
./scripts/test.sh --tcc --safety --no-build

# Read-only audit
./cmake-build-debug/TCCCompiler <module> --no-cache --safety-report
./cmake-build-debug/TCCCompiler <module> --no-cache --safety-report json

# Lower + verify without emitting/linking
./cmake-build-debug/TCCCompiler <module> --check --no-cache

# Audit baseline gate
./scripts/safety-audit.sh            # fail on growth
./scripts/safety-audit.sh --update   # record a reviewed baseline

# Compiler-size / audit-time baseline
./scripts/safety-baseline.sh
./scripts/safety-baseline.sh --update

# Main TCC regression
./scripts/test.sh --tcc --no-build

# Full relevant gate for safety/compiler changes
./scripts/test.sh --tcc
./scripts/test.sh --llvm
./scripts/test.sh --tcc --interpret
./scripts/test.sh --tcc --safety
./scripts/test.sh --tcc --negative
```

When compiler C++ changed, omit `--no-build` or rebuild first:

```bash
./scripts/build.sh --tcc -j 16
./scripts/build.sh --llvm -j 16
```

## Where safety work goes

| Area | Files |
|---|---|
| Diagnostic codes/formatting | `core/diag/Diagnostic.*`, `core/diag/Diagnoser.h`, `compiler/ASTDiagnoser.*` |
| Unsafe context/checks | `compiler/symres/SymResLinkBody.*`, `compiler/typeverify/TypeVerify.*` |
| Lifetimes/type facts | `compiler/safety/*` (new), `ast/types/*`, `parser/*` |
| MIR move/borrow/validity | `compiler/mir/MIRMoveCheck.*`, `MIRBorrowCheck.*`, `MIRValidity.*` (new) |
| Runtime checks | `compiler/mir/MIRRuntimeChecks.*`, C/LLVM emitters |
| Standard library | `lang/libs/core`, `lang/libs/std`, `lang/libs/cstd` |
| Tests | `lang/tests/negative/src/safety_*.ch`, `lang/tests/safety/` |
| Plan | `lang/docs/safety-implementation-plan.md` |
| Audit metrics | `lang/docs/safety/migration-metrics.md` |

## Current architecture reminders

- Backends never read the AST; all safety facts belong in MIR.
- MIR types are lifetime-erased; regions live in per-function side tables.
- `MIRMoveCheck` is the sole producer of init/drop state; elaboration inserts
  flags from its plan.
- Every emitted/interpreted `MIRFunction` (including lambdas, async poll/drop,
  stubs, comptime) must go through `lower_and_check`.
- Bounds lower as `Compare` + `CondBr` + panic call, not a mid-block opcode.
- C signed overflow/float→int must be lowered explicitly for TCC and LLVM.

## Review checklist for a safety change

- [ ] Stable `SAFE-*` code added, not reused.
- [ ] Report-only mode added first (unless runtime-only).
- [ ] Negative test asserts the code.
- [ ] Positive test compiles/runs.
- [ ] Unsafe-context counterpart behaves correctly.
- [ ] Libraries/tests migrated; zero report hits for the rule.
- [ ] TCC + LLVM + interpreter as applicable.
- [ ] `--safety-report` / `scripts/safety-audit.sh` updated.
- [ ] Plan status updated with what landed and what remains.
- [ ] No allowlist, no opt-out, no TODO/stub.
