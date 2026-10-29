---
name: Safety Implementation
description: How the Chemical safety system is implemented and tested — the full-parity plan, stable SAFE-* diagnostics, report→migrate→enforce workflow, safety test suites, and current implementation status. Load before implementing or reviewing any safety rule, borrow-checker change, or unsafe-boundary check.
---

# Chemical Safety Implementation

This skill is the working guide for the full safety parity program. The canonical
plan is [`lang/docs/safety-implementation-plan.md`](../../../lang/docs/safety-implementation-plan.md).
The source tree wins over any status note here.

## Current status (2026-10-28)

**M0 (S0.1–S0.6) and M3 are implemented. M1/M2/M4/M5 are advanced; M6 has started. S5.6b (unwinding) is withdrawn by policy: panic always aborts.**

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
  - `SAFE-UNSAFE-001` — dereference/projection/index/arithmetic/delete/
    dealloc through a **raw pointer** in safe context (enforced; dereferencing a
    reference is ordinary safe code and is not flagged).
  - `SAFE-PROV-001` — creating a reference from a raw pointer (`&*p`,
    `&mut *p`, or a raw/integer/null → reference cast) in safe context
    (enforced; S4.8).
  - `SAFE-UNSAFE-002` — call to an `unsafe func` or an unsafe/extern function
    pointer outside an `unsafe` context, and unsafe-to-safe function-pointer
    coercion (error; covers expression calls, methods, constructors, and
    variables/fields of function type).
  - `SAFE-UNSAFE-003` — inline assembly outside an `unsafe` boundary.
  - `SAFE-GLOBAL-001` — access to a mutable top-level global or `@extern`
    global outside an `unsafe` boundary (thread-local/comptime globals exempt).
  - `SAFE-GLOBAL-002` — immutable global whose type is not `Sync`.
  - `SAFE-ZERO-001` — safe zero-initialization of a type that does not implement
    `Zeroable`.
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
- S3.4/S4.6b dropck: the declaration-shape `SAFE-DROP-002` warning was retired.
  Dropck is per-use: a known callee returning a destructor-bearing aggregate
  from a single reference argument (a borrow guard) attaches that loan to the
  result place; `SAFE-LIFE-002` then rejects a guard whose drop is still live
  when the referent's storage ends. This follows variant/union payloads
  (`Option<Ref>`), so `RefCell` guards are covered, and it respects drop order
  (same-scope guard drops before the cell and is accepted).
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
  loses the result loan to the conservative escape path. When several arguments
  share the return region, the result carries an alias set of all candidate
  loans: every use keeps them live, and a return checks each candidate's
  region. Lambdas carry their enclosing function symbol; borrow
  diagnostics print `inside <parent>` and the source-location provider is
  installed even in `debug_quick`, so a generated lambda warning points at the
  enclosing function. Migrated the one in-tree two-`&mut` test; the tree now
  has zero `SAFE-BORROW-001` hits. `MIRTests` is `4632 checks`; negative tests
  cover aliased, ambiguous, and unrelated-argument cases (466 tests).
- S4.2b aggregate field regions + variance: `compiler/safety/Variance.h`
  computes declared variance (`&mut T`/`*mut T` pointee invariant,
  `Cell`/`UnsafeCell` invariant, unions invariant; function parameters
  contravariant) and the lifetime-parameter occurrences of a field type.
  `MIRRegionSignature::param_field_regions` records each direct reference field
  of an aggregate parameter (or a `&self` referent) with the interned field-name
  constant and the region selected through the parameter type's lifetime
  arguments (or the container's lifetime names for a method `&self`). The
  borrow checker seeds caller-owned field loans, resolves `p.field` / a method's
  `self.field` through a loaded reference parameter, and `SAFE-LIFE-003` now
  rejects returning the wrong lifetime argument. `region_signatures` is a
  mutex-copied deque so parallel lowering cannot invalidate a signature
  reference. S4.5 precision: borrow overlap uses constant field/array
  projections, so `a[0]`/`a[1]` are disjoint (even for `Copy` elements) while a
  dynamic index overlaps; `&mut *r` reborrows are accepted. `MIRTests` is
  `4638 checks`; negative tests `491`.
- S4.2b view/async soundness: borrowing views (`string_view`, `span`, `Chunk`)
  carry their source loan across `Call`/`Init` and aggregate returns, so a view
  of a local or `&raw` array storage cannot escape (`SAFE-LIFE-001`); a view of
  a parameter, a `'static`-returning allocator (`BatchAllocator.allocate_view`),
  or a same-scope local is accepted. `FutureHandle` fields are private and safe
  construction goes through the unsafe `future_handle_raw`/`set_raw` boundary,
  so the frame/vtable pair cannot be forged (`SAFE-VIS-001`). Async poll/resume
  functions treat reference frame fields as live loans and reject moving or
  dropping another frame field while the reference is still used
  (`SAFE-BORROW-002`); a borrow that ends before the move is accepted. A view
  built from a pointer-returning method (`s.data()`) inherits the receiver's
  loan too. Negative tests `498`; async suite `53`.
- S4.2/S1.2 temporary lifetime extension: a call result borrowed by a `var`
  initializer is registered as a block-local destructible instead of a
  statement temporary. The extended temporary is inserted before its owner in
  cleanup order, so `var h = H { r : &make() }` drops `h` first and keeps the
  temporary alive for `h`'s destructor. Extension intentionally does **not**
  apply through call arguments or later assignments; those temporaries end at
  the statement and `SAFE-LIFE-002` rejects a reference used after the end.
  Statement temporaries now emit `StorageDead`, and `FieldAddr` + `Load` reads
  count as uses of the stored field loan, covering pointer-based field access.
  Runtime tests check direct and aggregate destructor order; negative tests
  cover both non-extending forms. Safety suite `54`; negative tests `581`.
- S4.2 higher-ranked function types: `for<'a, 'b> (x : &'a T) => R` parses into
  `FunctionType::binder_lifetimes`; binders are scoped to the function type,
  duplicate binders are rejected, and lifetime-only generic function values
  (`func <'a> read(x : &'a T)`) instantiate once so they can be passed to a
  higher-ranked callback. Function-type satisfaction accepts elided/generic
  source lifetimes and rejects concrete `'static`/outer lifetimes.
  `MIRRegionSignature::hrtb_binders` records a declaration's own lifetime
  parameters and `MIRDump` prints them. The supported subset is a read-only
  callback with at most one direct shared borrowed parameter and no borrowed
  result; mutable/multiple/returning HRTB is rejected with `SAFE-BORROW-099`
  until full region inference. Safety suite `55`; negative tests `588`;
  `MIRTests` `4641 checks`.
- M5.5b float→int: `as` casts from float/double to integer widths lower to
  explicit blocks (NaN → 0, out-of-range → destination minimum/maximum) instead
  of C/LLVM `fptosi` UB. LLVM float `Ne` is `fcmp une`, matching C's
  `NaN != NaN`. Runtime tests cover saturation, truncation, and NaN.
- S5.6a panic runtime ABI: every module carries `__chc_panic`,
  `__chc_bounds_fail`, `__chc_overflow_fail`, `__chc_div_zero_fail`,
  `__chc_shift_fail` and `__chc_alloc_fail`. MIR checks pass the current
  file/line/column; the abort profile reports `panic: <reason> at
  <file>:<line>:<col>` on both C and LLVM. `std::panic::raw_panic` uses
  `__chc_panic`. Panic aborts by policy; there is no unwinding profile, so no unwind can cross FFI.
  Runtime tests assert the bounds reason and panic message; safety suite `50`,
  `MIRTests` `4638 checks`.
- S6.1a raw thread/async submission: `std::concurrent.spawn`,
  `ThreadPool.submit`/`submit_void`, and `async::spawn`/`spawn_local`/
  `spawn_blocking` are `unsafe` until erased-capture `Send`/`'static` tracking
  lands. `Thread`'s handle is private and only the unsafe `from_raw` constructs
  it. The parser/type-checker now honors `unsafe` on generic functions (the
  flag lives on the `GenericFuncDecl` master, not only a concrete copy), and
  `unsafe(...)` is valid as an expression statement. Negative tests `503`;
  TCC main `2351`, LLVM main `2349`. A compiler-side `span` implicit-overload /
  `intrinsics::size` nondeterminism is fixed by the M5.2a determinism slice.
- S1.4 unsafe interfaces + raw constructors: `unsafe interface` declares a
  contract whose implementor must write `unsafe impl` (`SAFE-TRAIT-001`);
  `core::iterable::Linear` is such an interface because `data()` feeds for-in
  pointer arithmetic; `Allocator` is unsafe because an impl supplies raw
  allocation pointers. Raw pointer/length constructors for `string`,
  `string_view`, `span`, and `u16string` are `unsafe func`, so safe code cannot
  forge a view of arbitrary memory; literal/reference constructors remain safe,
  and audited in-tree call sites wrap the trusted construction in `unsafe(...)`.
- Expressive-string printing: `intrinsics::expr_println` is an
  AST/comptime intrinsic that writes while the interpreter/comptime engine
  evaluates the expression. It is not a runtime print; compiled test output
  still uses `printf`/`sprintf` and therefore stays inside an explicit unsafe
  boundary. Do not move runtime diagnostics onto `expr_println`, which would
  print during compilation instead of execution.
- M5.3/S2.5 guards: `Layout` has private size/alignment with
  `size()`/`align()`/`validate()`; `GlobalAllocator` rejects zero/non-power-of-two
  alignment and sizes above `isize::MAX`. `SAFE-FFI-002` rejects `@extern`
  functions passing/returning struct/union/variant by value until target C ABI
  classification is implemented (pointer FFI is still allowed). Safety suite
  `55`, negative tests `588`.
- M5.2a validity/determinism: `SAFE-VALID-002` rejects non-bool to `bool`
  casts in safe code (atomic CAS uses `!= 0`); `SAFE-VALID-003` rejects an
  integer literal that is not a declared enum member; `SAFE-VALID-004` rejects
  `zeroed<Enum>()` when no member is 0. M5.2b is implemented: MIR lowering
  checks a runtime integer-to-enum cast against every declared member and calls
  `__chc_enum_fail` (panic) for an invalid tag. `SAFE-VALID-005` is retired;
  the literal-only `SAFE-VALID-003` check remains. `SAFE-VALID-006` requires
  an unsafe boundary for `@volatile` reads/writes: volatile is not atomic and
  concurrent access is a data race. `SAFE-VALID-007` requires an unsafe
  boundary for reading a named union field and reading a public anonymous-union field:
  the active union member is not tracked. In-tree lexer/parser/plugin
  casts were replaced with integer comparisons or total mapping helpers. `NonNull` has a private field and checked/unsafe
  constructors.
  `InterpretScope` now validates
  its identifier depth cache with a unique scope id and checks the current
  scope first; `resolve_ref` follows reference chains; array-returning
  single-return comptime calls fold through the AST; template arguments that
  are local consts use their AST initializer. The intermittent
  `span`/`string_view` `intrinsics::size` rejection is gone in repeated
  TCC/LLVM lib runs. Safety suite `55`, negative `588`; TCC main `2354`, LLVM
  main `2352`, libs `726`, plugins `1305`, async `53`, interpret `1910`.
- M5.7a interpreter memory guard: host `malloc`/`realloc` blocks have live and
  tombstone ranges, every read/write and `memcpy`/`memmove`/`memset` is
  range-checked, frame places are registered individually, and globals,
  constant blocks and comptime string buffers are registered. Violations set
  an interpreter error (no host abort). Each frame arena carries its owning
  interpreter's range table; this fixed nested-engine misregistration under
  some file orders. Negative tests cover comptime heap OOB and use-after-free.
- M5.7b interpreter boundary: every dereference must be inside a live
  registered allocation; unregistered/foreign pointers are rejected rather
  than falling back to host pointer sanity. Reset/destroyed frame ranges remain
  dead tombstones, so a pointer into a returned frame is rejected, and
  `strlen`/`strcmp`/`printf` use bounded scans with no host `std::strlen`
  fallback (`%s` is appended by checked length). Comptime string/aggregate args,
  `call_sret` buffers, and `argv` are registered as foreign ranges for the
  exact call/run lifetime. Negative tests cover foreign integer pointers,
  cross-frame pointers, and foreign `printf("%s")` arguments.
- M6 safe threads (first slice): `std::thread.spawn` accepts a non-capturing
  `(T) => U` entry with `T: Send + 'static`, `U: Send + 'static`;
  `spawn_void` is the void-returning form. `JoinHandle<T>` joins explicitly or
  from its destructor and is `Send` when `T: Send`; `Thread.join` is public.
  `T: 'static` is enforced for generic bounds (`SAFE-LIFE-004`), so a stack
  reference cannot be submitted. Conditional audited `Send`/`Sync` impls were
  added for `vector`, `deque`, `unordered_map`, and `ordered_map`. Capturing
  closures/erased captures still use the explicit unsafe
  `std::concurrent.spawn` path until M6.5. Guard-transfer negative tests cover
  `MutexGuard`/`RwLockWriteGuard`. `scripts/thread-tsan.sh` runs a four-thread
  `Arc<Mutex<int>>` smoke test under TSAN; it uses a system LLVM resource
  directory when the bundled prebuild lacks `libclang_rt.tsan.a`.
- S4.3b autoref / S4.7 two-phase: a reference receiver (`&self` / `&mut self`)
  now lowers to `Borrow`/`BorrowMut` instead of an untracked `AddressOf`, and
  method-call arguments are evaluated before the receiver borrow is activated.
  `v.push(v.len())` stays accepted (the shared argument loan ends first), while
  a live `&v` followed by `v.push(2)` is a hard `SAFE-BORROW-001`. Known-callee
  detection now uses the new `MIRSymbolRecord::has_body` (linkage is not body
  presence), and pre-declared top-level symbols carry their function type and
  region signature. Migrated `universal_cbi/router/emit.ch` to read an element
  before taking its mutable borrow.
- S2.2/S4.8 provenance enforced: `SAFE-UNSAFE-001` checks only raw-pointer
  dereferences (reference dereferences are safe and no longer warn), and
  `SAFE-PROV-001` covers `&*p`/`&mut *p` on a raw pointer and raw/integer/null
  → reference casts. The raw-pointer helpers were migrated by marking the
  affected parser/TLS/atomic/test helper bodies `unsafe { }` (expression-level
  `unsafe(...)` inside async functions, whose bodies the async lowering owns).
- S4.8 raw-pointer operations: `SAFE-UNSAFE-001` now also covers implicit
  dereference through a raw pointer. Field/method projection (`p.x`,
  `p.method()`), indexing (`p[i]`, including `vector.data()[i]`), pointer
  arithmetic (`p + n`, `p += n`, `p++`), and raw `delete`/`dealloc` require an
  unsafe boundary. The generated module-build scripts from `ModToLabConverter`
  carry one explicit unsafe boundary around their raw context/job operations.
  Wrapping generic bodies exposed a missing `ValueNode::copy`, now implemented.
  In-tree stdlib/CBI/plugin/build.lab call sites were migrated; audit baseline
  is `unsafe_blocks 573`, `unsafe_values 441`; negative `581`.
- S2.2/S4.8 function pointers: `FunctionTypeData::requires_unsafe` is part of
  function type identity and is set for `unsafe func` and `@extern`. Calling a
  value of such a type in safe context is `SAFE-UNSAFE-002`; coercing it to a
  safe function type is also rejected with `SAFE-UNSAFE-002`. `unsafe (args) =>
  ret` is the explicit unsafe function-pointer type. Negative tests cover
  variable calls, extern calls, coercion, and the `unsafe` type form.
- S2.2/S2.4 inline asm: `SAFE-UNSAFE-003` requires an unsafe boundary. Atomic
  fence helpers wrap each `asm` instruction explicitly; negative/positive tests
  cover the boundary.
- S5.5 C arithmetic: signed add/sub/mul/left-shift are emitted as unsigned
  computations cast back to the signed type, eliminating C signed-overflow UB
  and giving defined two's-complement wrapping in the default/release profile.
  `--overflow-checks` remains the opt-in checked debug profile.
- M2/global model: `SAFE-GLOBAL-001` gates mutable top-level `var` and
  `@extern` global access; `SAFE-GLOBAL-002` requires immutable globals to be
  `Sync`. Thread-local and comptime globals are exempt. In-tree call sites were
  migrated with audited unsafe boundaries or existing synchronization.
- `atomic_flag._value` is private; use `atomic_flag.make(value)` so safe code
  cannot forge the flag storage.
- Named unsafe flags (`unsafe "no_lifetime_check"`) are rejected. `unsafe` is a
  boundary marker only; it does not disable lifetime checking.
- S5.1 Zeroable: `core::Zeroable` is an `unsafe interface`; safe
  `zeroed<T>()`/`= zeroed` require `T: Zeroable` and report `SAFE-ZERO-001`
  otherwise. Builtins, raw pointers, arrays, plain all-Zeroable structs, and
  zero-member enums are structural; constructor/destructor/invariant types
  require an explicit audited `unsafe impl Zeroable`. `MaybeUninit<T>` is
  unconditionally Zeroable, `ManuallyDrop<T>` requires `T: Zeroable`, and
  `vector::resize` now carries a `T: Zeroable` bound. `@allow_zeroed` is
  removed; production call sites use the explicit unsafe
  `std::mem.zeroed_unchecked<T>()`, with `zeroed:unsafe` left only as the
  audited bootstrap inside that helper and a few generic/array test cases.
- S4.1 (partial) Pin/Unpin: `core::Unpin` is a safe auto trait derived from
  fields (pointers/references are Unpin; negative impls are allowed), and
  `core::PhantomPinned` opts out via `impl !Unpin`. `core::Pin<T>` has a
  private pointer, unsafe `from_raw`, `new`/`pin(&mut T)` (`T: Unpin`), safe
  `get_ref`, `get_mut`
  (`T: Unpin`), `unsafe get_unchecked_mut`, and `into_inner`. `Waker` is
  explicitly `unsafe impl Send`/`Sync` because its vtable contract requires
  thread-safe wake/clone/drop. Its data/vtable fields are private: safe code
  uses `Waker.empty()` (null-safe wake/clone) or `unsafe Waker.from_raw(...)`
  for a real task, and `is_empty()` replaces raw vtable checks. Struct-literal
  initialization of a private field outside its type is `SAFE-VIS-001`, so the
  fields cannot be bypassed. Async frame field checking remains.
- M5.1 value validity (zeroed): `SAFE-VALID-001` rejects `zeroed<T>()` when
  the concrete type contains a reference field (directly, in an array, or in a
  variant payload): zeroing would create a null reference. Specialized generic
  fields (`W<&int>`) are checked too; `PhantomData<&T>` is storage-free and
  exempt. `zeroed<T>()` now requires `T: Zeroable`; audited internals use the
  explicit unsafe `std::mem.zeroed_unchecked<T>()`. Negative tests
  cover direct/generic reference rejection, a plain struct, and PhantomData.
- S2.6 FFI enforced: `SAFE-FFI-001` is a hard error for a call to an
  `@extern` function from a safe context (the callee's contract is invisible
  to the compiler). Every in-tree call site is now inside a per-call
  `unsafe(...)` expression or `unsafe { ... }` statement block (883 library
  calls plus test/build.lab call sites); async functions use expression-level
  unsafe because the async lowering owns their bodies. The negative test now
  expects the error, and `extern_call_in_unsafe_ok` covers the accepted form.
- S4.6b per-use dropck: a known callee returning a destructor-bearing aggregate
  from one reference argument binds that loan to the result place; returning a
  `Ref`/`RefMut` (including through `Option`) keeps the loan live until the
  guard's drop, so `SAFE-LIFE-002` rejects a guard that outlives its referent.
  The old declaration-shape `SAFE-DROP-002` warning is retired.
- S4.9/S4.10 enforcement: after the tree reached zero hits across every suite,
  `SAFE-BORROW-001` and `SAFE-BORROW-002` were flipped to hard errors. The
  enforcement gate is the same fatal path as the LIFE rules; enforced
  diagnostics render as `error[SAFE-*]`. The negative suite asserts the error
  form for alias, shared/mutable alias, move-while-borrowed, single- and
  multi-reference result loans (`460` tests).
  S4.2b variance and temporary lifetime extension are now implemented; HRTB,
  dropck/async/Pin precision, and remaining provenance work stay in M4.
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

- M6.5: erased/async `Send`. Scoped threads are
  blocked on full M4 HRTB/region inference: a body-local reference must not
  outlive the scope join, which needs a higher-ranked scope lifetime for
  capturing/multi-borrow callbacks. Read-only single-borrow function-type HRTB
  is implemented; mutable/returning/multi-borrow callbacks are rejected with
  `SAFE-BORROW-099`. Capturing
  closures and non-`'static` scoped borrows remain on the explicit unsafe path.
  Then remaining M4 precision, M5 packed references, and M2 C ABI.
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
  (S1.5l). `std::NonNull<T>` (S1.6b) has a private field, safe
  `non_null`/`non_null_try`, unsafe `non_null_unchecked`, and is deliberately
  neither `Send` nor `Sync`. `Box`, `Rc`/`Weak`, and `Arc`/`ArcWeak` also keep
  their pointer fields private; `vector` storage is internal and `string`/
  `u16string` tagged storage is private. `std::MaybeUninit<T>` and
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
  locals/globals/fields/derefs/elements. Checked signed `+`, `-`, `*` and
  unary negation (S5.5b) run under `--overflow-checks`, including compound
  assignments; add/sub use the sign-bit identity, mul a guarded quotient, and
  a literal `-2147483648i32` remains valid while a runtime `-MIN` traps.
  Unsigned arithmetic keeps defined wrapping and the flag is off by default.
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
| `SAFE-BORROW-099` | unsupported borrow/lifetime pattern (for example a mutable or borrowed-result HRTB callback) |
| `SAFE-BORROW-*` | aliasing / loans |
| `SAFE-VALID-001` | zero-initialization would create a null reference |
| `SAFE-VALID-002` | non-bool to `bool` cast in safe code |
| `SAFE-VALID-003` | integer literal is not a declared enum member |
| `SAFE-VALID-004` | `zeroed<Enum>()` has no declared zero tag |
| `SAFE-VALID-005` | retired: runtime int-to-enum casts are now checked in MIR |
| `SAFE-VALID-006` | `@volatile` access outside an unsafe boundary |
| `SAFE-VALID-007` | union field read outside an unsafe boundary |
| `SAFE-TRAIT-001` | implementation of an `unsafe interface` was not `unsafe impl` |
| `SAFE-VIS-001` | private/internal member initialized or accessed outside its type/module |
| `SAFE-FFI-001` | extern call in a safe context (enforced) |
| `SAFE-COPY-001` | user type used with a `Copy` bound but no explicit `impl Copy` |
| `SAFE-COPY-002` | `impl Copy` on a type with a destructor |
| `SAFE-COPY-003` | `impl Copy` on a type whose field is not Copy |
| `SAFE-DROP-001` | invalid destructor signature (value self, missing self, non-void return, async) |
| `SAFE-DROP-002` | retired: dropck is per-use via `SAFE-LIFE-002` (do not reuse) |
| `SAFE-AUTO-001` | `Send`/`Sync` implemented without `unsafe impl` |
| `SAFE-AUTO-002` | invalid/conflicting negative impl (non-auto trait, duplicate, positive+negative, generic target) |
| `SAFE-MOVE-001` | move of an already-moved value / access or assignment after a move |
| `SAFE-LIFE-001` | returning a reference to a local/temporary/frame field |
| `SAFE-LIFE-002` | borrow still live when the referent's local storage ends |
| `SAFE-LIFE-003` | returned reference does not outlive the declared return lifetime |
| `SAFE-LIFE-004` | a type does not satisfy a `'static` generic bound (e.g. a stack reference submitted to a thread) |
| `SAFE-LIFE-*` | lifetimes / escapes |
| `SAFE-INIT-*` | initialization / definite assignment |
| `SAFE-VALID-*` | invalid values / transmute / packed |
| `SAFE-BOUNDS-*` | indexing |
| `SAFE-OVERFLOW-*` | arithmetic |
| `SAFE-SEND-*` | Send/Sync / races |
| `SAFE-UNSAFE-*` | unsafe boundary |
| `SAFE-PROV-001` | reference created from a raw pointer in safe context (provenance, enforced) |
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

# M6 thread data-race smoke under TSAN (system LLVM resources)
./scripts/thread-tsan.sh

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
