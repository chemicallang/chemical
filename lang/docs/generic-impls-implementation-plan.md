# Generic impls — implementation plan (v2)

> Status: **plan / not started** · Proposal: [`proposals/0003-generic-impls.md`](proposals/0003-generic-impls.md)
> Unblocks: [`0014-generic-container-serialization.md`](proposals/0014-generic-container-serialization.md)
>
> v2 incorporates an adversarial review of the code paths; the deltas vs. v1 are
> marked **[review]** where they change a decision.

## 1. Executive summary

Top-level `impl<T…> Trait for Type<T…>` is the missing language capability. Much
of the machinery already exists — `GenericImplDecl`, its finalization hooks,
dedup, and pass visitors — but the parser never creates a `GenericImplDecl`: it
reports an error and **discards** the parameter list
(`parser/structures/ImplDef.cpp:26-43`). Everything downstream is unreachable
dead code.

Turning the parser on is not sufficient. Three structural problems must be
solved:

1. **Lazy, demand-driven instantiation.** Eager instantiation would instantiate
   `Serializer for vector<Foo>` for every `vector<Foo>`, even when nobody
   serializes it — wrong bound errors, compile-time cost, and code bloat. This
   violates the product requirement "cost only when used".
2. **Target-pattern unification.** The deserializer shape
   `impl<T> Deserializer<vector<T>> for TypeDecoder<vector<T>>` has a nested
   generic target; identity mapping is not enough.
3. **Correct publicatation and linkage of instantiated impls.** The existing
   `GenericImplDecl` code assumes the master impl's types were linked (they are
   not), does not substitute `interface_type`/`struct_type`, races
   `body_linked` with on-demand registration, and does not recreate the
   `VisitImplDecl` symbol environment in instantiated bodies. **[review]**

**Design in one sentence:** register generic impls keyed by their generic target
declaration; keep them dormant; when resolution demands a member or interface
impl on a concrete generic target, match the target pattern, check bounds,
instantiate exactly that impl for exactly those arguments, adopt + publish it
under a status protocol, and retry the lookup.

---

## 2. Current state (evidence)

| Piece | Status |
|---|---|
| `GenericImplDecl` AST node | **Present** (`ast/structures/GenericImplDecl.{h,cpp}`) |
| Declare/link-body/typecheck visitors | **Present** (`DeclareTopLevel.cpp:159`, `SymResLinkBody.cpp:1710`, `TypeVerify.cpp:2119`) |
| Generic instantiation finalization | **Present** (`GenericInstantiator.cpp:1507`, `:1542`) |
| `register_generic_args` + dedup | **Present** (`GenericImplDecl.cpp:47`) |
| Nested-impl adoption/index helper | **Present** (`handle_new_impl`, `GenericInstantiator.cpp:963`) |
| Parser creates `GenericImplDecl` | **Missing** — rejects and discards params |
| Link signature of the master `interface_type`/`struct_type` | **Missing** — `VisitGenericImplDecl` only links params + members (`LinkSignature.cpp:1078-1086`, contrast non-generic at `:1256-1265`) **[review]** |
| Registry of generic impls by target | **Missing** |
| Trigger to instantiate generic impls | **Missing** (no caller of `register_generic_args`) |
| Copy/substitute impl `interface_type`/`struct_type` | **Missing** (`GenericImplDecl::finalize_signature`, `FinalizeSignature`) |
| Body-environment recreation on instantiation | **Missing** (`FinalizeBody` only declares impl locals/functions) **[review]** |
| `body_linked` / `instantiations` synchronization | **Broken for on-demand registration** (`GenericImplDecl.h:59`, `SymResLinkBody.cpp:1722-1730`) **[review]** |
| Tests | **Missing**; two negative tests assert the old rejection |

Nested impls inside generic structs prove the downstream path: `ordered_map`
already has nested `core::iterable::Iterable` impls (`ordered_map.ch:212`) that
are instantiated, adopted and dispatched correctly.

---

## 3. Goals, non-goals, invariants

### Goals

- Top-level `impl<T…> Trait for Target<T…>` compiles, resolves methods,
  supports interface satisfaction/vtables, operators and iteration, on **TCC,
  LLVM and the MIR interpreter**.
- Multiple parameters, inline bounds (`impl<T : Trait, U> …`), nested target
  patterns (`TypeDecoder<vector<T>>`), and concrete pattern arguments where
  needed (`ordered_map<std::string, V>`).
- Bounds are checked **at the demand site**, with an actionable diagnostic.
- Generic container serialization (`vector`, `span`, maps) and `#json` support.
- Exhaustive tests, including *laziness*, *dedup*, *cross-module*, *clean-vs-cache*
  and *no duplicate vtable* tests.

### Non-goals (v1)

- Runtime dictionary/typeclass dispatch.
- Higher-kinded types, associated types, specialisation, negative reasoning.
- Generic impls on primitive/non-generic targets (`impl<T> Foo for int` is
  rejected: `T` is unused).
- **Structural target patterns** (`impl<T> Foo for *T`, `&T`, `[N]T`): the
  compiler cannot index pointer/reference impls (`LinkSignature.cpp:1599-1615`)
  and has no array implementation key. Only named generic struct/union/variant
  targets. **[review]**
- `where` clauses on impl headers. Inline `<T : A + B>` only. If a `where`
  clause is parsed on an impl, reject it explicitly rather than silently dropping
  it (`AfterBuildIndexesPass` already handles function where clauses,
  `LinkSignature.cpp:1952-1959`). **[review]**
- Bounds on **generic struct parameters** (e.g. `vector<T>`'s own bounds). The
  existing compiler does not enforce them; do not change that here.
- Parenthesised/composite target patterns beyond named generic types and
  pointers/references *inside arguments* (arguments may be nested generic types;
  the target itself must be a named generic declaration).

### Invariants

- **I1 — Zero cost when unused.** An uninstantiated generic impl emits **no
  symbols**, allocates no concrete `ImplDefinition`, and performs no
  signature/body work. A type that merely matches its target pays one registry
  check.
- **I2 — One instance per (impl, canonical args).** Dedup is global and
  representation-insensitive (`GenericType` vs relinked concrete
  `StructDefinition` must compare equal). **[review]**
- **I3 — Bound failure only on demand.** `impl<T : Serializer> S for Box<T>`
  must not reject `Box<int>` unless that impl is needed.
- **I4 — Backend identity.** TCC, LLVM and interpreter produce identical
  observable behaviour, including `dyn` over instantiated impls; backends read
  MIR only.
- **I5 — Shared-state safety.** Registration, body finalization, adoption and
  lookup are safe under parallel file symbol resolution; no lost updates, no
  iterator invalidation, no unsynchronized map mutation. **[review]**

---

## 4. Design

### 4.1 Pipeline timing and registration

The pipeline is: all `tld_declare` (serial) → all link signatures (parallel) →
`generate_automatic_functions` → generic instantiation pass (parallel) → after
link signature (`BuildIndexes`) → body pass 1 (generic masters, parallel) → body
pass 2 (parallel).

Registration must be complete **before** the generic-instantiation pass. The
link-signature pass is parallel, so registering from `VisitGenericImplDecl` is
still ordered before the pass ends, but does not help other files' link-signature
comptime expressions. **[review]** Add a tiny serial module pass between link
signatures and generic instantiation:

```
sym_res_register_generic_impls(module):
  for each file / top-level node (recurse namespaces + taken comptime ifs):
    if GenericImplDecl -> validate + register
```

This is a one-time top-level declaration scan (no per-instantiation cost). If
keeping registration in `VisitGenericImplDecl` is preferred, the same validation
function runs there; the serial pass only handles ordering. **[review]**

Link-signature comptime expressions that demand a generic impl from a file not
yet registered remain a documented limitation; a pending-demand queue drained
after link signatures is a stretch goal (not v1).

### 4.2 Registry

Extend `ImplementationsIndex` (job-global) with:

```cpp
std::unordered_map<ASTNode*, std::vector<GenericImplDecl*>> generic_impls_;
std::atomic<size_t> generic_impl_count_{0};
mutable std::shared_mutex generic_impl_mutex_;   // registry + adoption visibility
std::recursive_mutex generic_impl_adopt_mutex_;  // serialise adoption [review]
```

- `add_generic_impl(target, impl)`: unique lock; identical-pattern duplicate is
  an immediate error; append + increment.
- `snapshot_generic_impls(target)`: shared lock, copy the small candidate vector
  (do **not** instantiate while holding it).
- `has_any_generic_impls()`: relaxed atomic load — the zero-cost fast path.
- **`clear()` must reset the registry, the counter and both mutexes' state**
  (it is called per job, `LabBuildCompiler.cpp:224,285`). **[review]**

Visibility respect: registration records the impl's access/module; demand checks
that a non-public impl is only visible from its module. **[review]**

### 4.3 Target patterns

The target pattern is `master_impl->struct_type` (a `GenericType`).

Supported v1 patterns:

```
T                                  bind a generic parameter
Wrapper<T>                         nested named generic pattern
ordered_map<std::string, V>        concrete + generic arguments
primitive argument types           structural equality
```

The **target itself** must be a named generic struct/union/variant. Pointer,
reference and array target patterns are rejected in v1. **[review]**

Matcher:

```cpp
enum class MatchResult { NoMatch, Match, Defer };

MatchResult match_pattern(
    BaseType* pattern,
    BaseType* actual,
    std::unordered_map<GenericTypeParameter*, BaseType*>& bindings);
```

- `Defer` when `actual` still contains an unresolved `GenericTypeParam` (generic
  master body). Retried after substitution.
- Concrete generic arguments are resolved to `(generic declaration, args)` via
  `get_direct_linked_node()`; if the node is a concrete `StructDefinition` with
  `generic_parent`, recover args from `InstantiationsContainer` by
  `generic_instantiation`.
- Bound parameters must be consistent (`BaseType::is_same`). Non-linear patterns
  `Pair<T, T>` are rejected at registration (no unification of repeated
  placeholders in v1).

### 4.4 Instantiation status, locking and re-entrancy **[review]**

`body_linked` and the `instantiations` vector are currently unsynchronised
(`GenericImplDecl.h:59`, `SymResLinkBody.cpp:1722-1730`). On-demand registration
from parallel body/typeverify passes makes this a real race and a lost-update
bug (an instantiation appended just before `body_linked = true` is never
body-finalized).

Add a per-instantiation status sequence stored beside `instantiations`:

```
Registered -> SignatureFinalized -> Adopted -> BodyFinalized
```

Rules:

- **Register** under `registration_mutex`; append under the lock.
- **Signature** finalization outside the lock (existing).
- **Adopt** under `generic_impl_adopt_mutex_` (a *recursive* mutex, because
  nested instantiation can re-enter), then publish `Adopted` and notify.
- **Body link** (`VisitGenericImplDecl`): under `registration_mutex`, snapshot
  `instantiations`, set `body_linked = true`, release; finalize the snapshot.
  A registration after that snapshot sees `body_linked == true` and finalizes
  its own body. A registration before it is in the snapshot. Each instantiation
  body is finalized at most once, guarded by the status.
- **Wait:** a demand returns only after `Adopted`. Existing `register_generic_args`
  returns the existing instance before that (`GenericImplDecl.cpp:61-75`);
  callers that need the methods must wait for `Adopted`, not
  `SignatureFinalized`. Same-thread recursion is exempt from waiting.
- **Same-thread pending:** if the demanded impl is the one currently being
  created on this thread, do not wait. Bounds treat it as provisionally
  satisfied; method lookup defers the child (§4.6). A genuine infinite expansion
  is reported with a recursion diagnostic, not a deadlock.
- **Lookups** on a concrete generic container must take a **shared lock** around
  `MembersContainer::indexes` access, because adoption mutates it concurrently.
  Non-generic containers take no lock. Add a `std::shared_mutex*` to
  `ChildResolver` (set by the resolver) and use it in
  `child_provider_linked_type`; `ASTNode::child` stays lock-free for all
  non-generic paths. **[review]**
- **`InstantiationsContainer::getInstantiationTypesFor` is unsynchronised**
  (`InstantiationsContainer.h:85-90`). Read concrete args under the existing
  `generic_inst_reg_mutex` (the same mutex `activateIteration` takes,
  `GenericInstantiator.cpp:762-766`), or add a safe accessor. **[review]**
- **Failure rollback:** `register_generic_args` publishes before signature
  finalization (`GenericImplDecl.cpp:77-101`). A failed signature/bound must
  mark the instance `Failed` or remove it from `current_module_instantiations`
  so codegen never sees a half-built impl. **[review]**

### 4.5 Demand sites

`ensure_generic_impls(for_, interface_filter, member_filter, demand_loc)` runs
only when `has_any_generic_impls()`.

| Context | Hook |
|---|---|
| Member access/method call | `find_link_in_parent` (`SymResLinkBody.cpp:493`): `provide_child` miss → ensure → retry |
| Member access while instantiating a generic body | `GenericInstantiator::relink_identifier` / `relink_parent` (`GenericInstantiator.cpp:79`, `:156`) |
| Operators (binary, assignment, index, inc/dec, unary) | `SymResLinkBody::VisitExpression`, plus `GenericInstantiator::VisitExpression`, `VisitIndexOperator`, `VisitIncDecValue`, `VisitNegativeValue`, `VisitNotValue`, `VisitBitwiseNot` **[review]** |
| `for x in value` | `SymResLinkBody::VisitForInLoopStmt` before `get_linear_*` / `get_iterable_*` / `get_chunked_*`; `MIRLowerStmt.cpp:1834-1866` must only fail if symres left a gap **[review]** |
| Where-clause / interface satisfaction | `TypeVerifier` on `get_impl` miss (`TypeVerify.cpp:1086`, `:1401`), with a per-file demand callback |
| `dyn` coercion / `satisfies` | `LinkedType::satisfies` (`ast/types/LinkedType.cpp:122-130`) and `GenericType::satisfies` (`GenericType.cpp:218-239`) consult `users`, not `get_impl`; ensure the impl is adopted before these run (typeverify demand) **[review]** |
| Comptime expression folding | `InterpretScope.cpp:451-508`, `GlobalFunctions.cpp` stream helpers; lower priority, same service **[review]** |

Const plumbing: `Expression::get_overloaded_func`/`get_determined_type` and the
value-level `determine_type`s take `const ImplementationsIndex&`, and
`SymResLinkBody::getImplsIndex()` is const. Demand needs mutable access; either
pre-demand in `SymResLinkBody::VisitExpression` (preferred) or add a
`const_cast`-free mutable accessor. Do not thread a mutating service through
every value node. **[review]**

Member filtering must also consider `InterfaceDefinition::extension_functions`
(indexed by `build_indexes_of_impl`, `LinkSignature.cpp:1705-1716`). **[review]**

### 4.6 Deferred unresolved children in generic bodies **[review]**

`find_link_in_parent` currently reports `unresolved child` even in
`generic_context` (`SymResLinkBody.cpp:493-507`). A generic impl body calling a
method supplied by a *different* generic impl for the same target would fail at
master-body link time, before instantiation can resolve it.

When `provide_child` misses and all of these hold:

- `generic_context` is true;
- the receiver type contains an unresolved `GenericTypeParam`;
- the target generic declaration has registered impl candidates;

then link the identifier to an unresolved placeholder, **suppress the error**,
and mark the chain/identifier as deferred. `GenericInstantiator::relink_parent`
already re-resolves chain members after substitution, and `relink_identifier`
after substitution; both retry `ensure_generic_impls`. Type verification must not
error on the deferred marker (skip the expression until instantiated).
Add a regression test where one generic impl calls a method supplied by a second
generic impl for the same target.

### 4.7 Bounds

For every `GenericTypeParameter` of the impl with `traits`:

1. Resolve the trait to an interface; accept **both** `InterfaceDefinition` and
   `GenericInterfaceDecl` (`generic_parent->master_impl`). The existing
   where-clause verifier silently skips generic interfaces
   (`TypeVerify.cpp:1063-1066`, `LinkSignature.cpp:1875-1884`); use one shared
   bound-check helper for both. **[review]**
2. Get the concrete argument's members container.
3. `implsIndex.get_impl(interface, container)`; if missing, recursively
   `ensure_generic_impls` for that argument (guarded) and retry.
4. If still missing, report at `demand_loc`:

```
error: type 'Foo' does not satisfy bound 'std::Serializer<JsonValue, JsonEncoder>'
       required by impl<T : ...> ... for vector<T>
note: first required here
```

Cycle policy: a same-thread `Registered/SignatureFinalized` impl counts as
provisionally satisfying its own bounds (recursive types are legal); only a
genuine unfulfilled bound is an error. **[review]**

Do not check bounds for generic *struct* parameters.

### 4.8 Dedup identity and concrete-impl precedence **[review]**

Dedup currently distinguishes a `GenericType` argument from a
`LinkedType`/concrete `StructDefinition` for the same logical type
(`ASTUtils.cpp:182-233`; `LinkedType::is_same` at `LinkedType.cpp:60-63`). Demand
can derive args in either representation, producing two instances with the same
mangled name (`__cgs__N`, `NameMangler.cpp:22-30`) and duplicate definitions.

- Normalize args before registration with one canonicalisation helper (reuse
  `canonicalize_instantiation_arg`, extended to resolve a concrete generic
  `StructDefinition` back to its `(generic parent, arg list)`).
- Before instantiating a generic impl, check
  `implsIndex.get_impl(interface, target_container)`: an existing **concrete**
  impl wins; suppress the generic instantiation entirely (no adoption, no
  emission). This also fixes a silent duplicate in the current `emplace`
  behaviour (`ImplementationsIndex.h:55-58`).
- Test both representations explicitly.

### 4.9 Adoption and the instantiated body environment **[review]**

`GenericImplDecl::finalize_signature` must:

- copy `interface_type` and `struct_type` (they are currently shared with the
  master);
- copy inherited types/variables/functions as today;
- handle nested `ImplDecl` children (currently only top-level functions).

`GenericInstantiator::FinalizeSignature(GenericImplDecl*)` must visit the copied
`interface_type` and `struct_type` (mirror `FinalizeNestedImplSignature` at
`GenericInstantiator.cpp:1429-1461`).

`FinalizeBody(GenericImplDecl*)` must recreate the `VisitImplDecl` environment
(`SymResLinkBody.cpp:1922-1951`), not just the impl's own locals/functions:

- declare/redeclare the target container's variables and inherited members in
  the symbol table (unqualified field access in impl methods);
- declare the interface's functions, including defaults and extension functions;
- adopt the interface into the target (`struct_linked->adopt(linked)`) and run
  the equivalent of `build_indexes_of_impl` so extension/default methods resolve;
- handle nested `ImplDecl` children (signature and body);
- resolve generic interfaces via `generic_parent->master_impl` where the
  non-generic API expects `InterfaceDecl`.

Adoption sequence (after signature finalization):

1. `target_container->adopt(impl)` (method names into `indexes`);
2. interface default/extension adoption (`struct_linked->adopt(interface)` and
   `build_indexes_of_impl` equivalent);
3. `interface->register_impl(impl)` (vtable users, `has_implementation`);
4. `implsIndex.add_interface(interface, target_container, impl)`;
5. publish `Adopted`, notify.

Steps 1–2 mutate the target container; take `generic_impl_adopt_mutex_` and the
registry's write lock as needed (§4.4). Steps 3–4 have their own lock
(`ImplementationsIndex::add_interface` takes `index_mutex`).

### 4.10 Codegen, cache, interpreter **[review]**

Once instantiated, a generic impl is an ordinary `ImplDefinition` in
`current_module_instantiations`. The existing MIR path handles it:
`mir_decl.declare_all`, `process_nodes` → `lower_impl`, vtables, globals.

Required fixes:

- Pass `container.get_current_module_instantiations()` to
  `build_module_vtables` (`ASTProcessor.cpp:2889-2901`) for **both** LLVM and the
  interpreter: `MIRInterpreter` resolves `DynamicCall` through
  `module.vtables` (`MIRInterpreter.cpp:521-528`). Do not frame this as
  LLVM-only.
- `set_generated_instantiations(GenericImplDecl)` is a **no-op** — the counters
  it writes (`total_*_instantiations`) have no readers and it cannot see the
  concrete `ImplDefinition` nodes. Replace it with cache handling on the
  `current_module_instantiations` list: mark `ImplDecl`/`FunctionDecl` nodes the
  cached module defined, and mark the interface's `vtable_pointers` if a vtable
  was emitted. **[review]**
- Duplicate-emission guard: an impl is emitted once because it lives in exactly
  one module's instantiation list. Make this explicit in the cache/declaration
  path (`set_defined_declarations` has no `ImplDecl` case) and test it across
  modules and clean-vs-cached builds.
- Cache determinism: `__cgs__N` comes from registration order. Lazy demand adds
  registration points; a cached module can encode a different suffix from a
  fresh build. Test `nm`/`--emit-c` symbol identity between a clean build and an
  incremental `--cache` rebuild.

### 4.11 Cost model

| Situation | Cost |
|---|---|
| No generic impl in the program | One relaxed atomic load on relevant lookups; no map, no lock, no allocation |
| Generic impl declared, never used | One registry entry; no `ImplDefinition`, no signature/body work, no symbols |
| Generic impl used for N concrete arg tuples | N concrete impls + methods, exactly like N hand-written impls; existing hash dedup |
| Lookup on a non-generic container | No lock, unchanged path |
| Lookup on a concrete generic container | One shared-lock acquisition + map lookup, only if the target has generic impls |

Explicitly avoided: whole-program per-type scans, eager instantiation, runtime
dictionaries, string-keyed dispatch, per-container mutex storage.

---

## 5. Work breakdown

### Phase 0 — Baseline and harness

- Record current `--tcc`, `--llvm`, `--interpret`, `--plugins`, `--negative`
  counts, compile time, peak RSS, `tests-tcc.exe` size.
- Add the test skeletons (§6) and the `--emit-c` helper needed by the laziness
  test.

Gate: baselines reproducible.

### Phase 1 — Parser, AST, linkage

Files: `parser/structures/ImplDef.cpp`,
`ast/structures/GenericImplDecl.{h,cpp}`, `compiler/symres/LinkSignature.cpp`.

- Create `GenericImplDecl` when `<` follows `impl`; set `parent_node`,
  `generic_parent`, generic params; keep the non-generic path identical.
- Generic impl master bodies use the job/global allocator and are body-retained
  (mirror generic structs; function parsing marks them retained via the allocator
  equality rule, `Function.cpp:642-645`).
- **Explicitly link `master_impl->interface_type` and `master_impl->struct_type`
  inside the generic parameter scope** (`LinkSignature.cpp:1256-1265` shows the
  non-generic order). Without this, `struct_type->referenced->linked` stays null
  and `GenericInstantiator::VisitGenericType` dereferences null
  (`GenericInstantiator.cpp:511-514`). **[review]**
- Keep `GenericInstantiationPass::VisitGenericImplDecl` a no-op (no eager work).
- Guard `Self` as a target (`LexType.cpp:392-399` currently falls back to the
  interface when `struct_type` is null) and reject it for generic impls with a
  dedicated diagnostic. **[review]**

Gate: positive declarations compile on all three backends; old rejection tests
are converted or removed.

### Phase 2 — Registration, validation, matcher

Files: `ImplementationsIndex.{h,cpp}`, a new serial registration pass,
`GenericImplDecl.{h,cpp}`.

- Implement the registry, atomic fast path, clearing, visibility.
- Validate: generic target, every param used, no non-linear patterns, argument
  count, duplicate identical patterns.
- Implement `match_pattern` + concrete-argument recovery.
- Do not instantiate.

Tests: negative suite for invalid declarations; no symbols emitted.

### Phase 3 — Lazy method demand, status protocol, body environment

Files: `GenericInstantiator.{h,cpp}`, `GenInstantiatorAPI.h`,
`SymResLinkBody.cpp`, `GenericImplDecl.cpp`, `ASTNode.cpp`, `ChildResolver.h`.

- `ensure_generic_impls`, candidate filtering, bounds.
- Status sequence + waits + body snapshot/publish (§4.4).
- Copy/visit interface+struct types; recreate the body environment; nested impls.
- Adoption/indexing under locks; shared-lock lookups for concrete generic
  containers.
- Canonical dedup + concrete-impl precedence.
- Deferred unresolved children (§4.6).
- Retry in `find_link_in_parent`, `relink_identifier`, `relink_parent`.

Tests: common method tests, bounds, laziness, dedup representations, cross-module
dedup, destructor counts. All three backends.

Gate: method/impl tests green; laziness symbol test; no data-race reports under a
threaded `--tcc` build.

### Phase 4 — Operators, iteration, interface satisfaction

Files: `SymResLinkBody.cpp` (`VisitExpression`, `VisitForInLoopStmt`),
`GenericInstantiator.cpp` (operator visits), `TypeVerifier.{h,cpp}`,
`TypeVerifyAPI.h`, `ASTProcessor.cpp`, `LinkedType.cpp`/`GenericType.cpp` if
needed.

- Pre-demand in operator/for-in resolution at both symres and generic-body
  finalization.
- Per-file typeverify demand callback (constructed only when the registry is
  non-empty).
- Shared bound helper for generic interfaces.
- `dyn`/`satisfies` adoption before typeverify.
- Ambiguity diagnostics; deterministic ordering.

Tests: generic `Add`, generic iterable/linear, `dyn` coercion, `where` clauses,
static interface, inherited interface, default/extension methods.

Gate: all three backends; non-generic operator/iteration suites unchanged.

### Phase 5 — Codegen, interpreter, cache hardening

- Current instantiations into `build_module_vtables` (LLVM + interpreter).
- Cache marking for `ImplDecl`/interface vtables; remove the no-op
  `GenericImplDecl` item.
- Duplicate-emission guard + tests.
- Clean-vs-cached symbol identity test.
- Two-module link test (define/use split) and three-module dedup test.

Gate: no duplicate symbols/vtables, cached and clean agree, interpreter `dyn`
works.

### Phase 6 — Generic container serialization

New file `lang/libs/json/src/containers.ch` (json may depend on std; std must not
depend on json):

```chemical
impl<T : std::Serializer<JsonValue, JsonEncoder>>
    std::Serializer<JsonValue, JsonEncoder> for std::vector<T> { … }

impl<T>
    std::Deserializer<std::vector<T>> for TypeDecoder<std::vector<T>> { … }

impl<T : std::Serializer<JsonValue, JsonEncoder>>
    std::Serializer<JsonValue, JsonEncoder> for std::span<T> { … }   // encode only

impl<V : std::Serializer<JsonValue, JsonEncoder>>
    std::Serializer<JsonValue, JsonEncoder> for std::ordered_map<std::string, V> { … }

impl<V>
    std::Deserializer<std::ordered_map<std::string, V>>
    for TypeDecoder<std::ordered_map<std::string, V>> { … }

// same pair for unordered_map
```

Notes:

- Vector encode: `encoder.array()`, iterate `size()`/`get_ptr(i)`, call
  `arr.encode<T>(ptr)`, propagate the first `Err`.
- Vector decode: `self.decoder.array()`, iterate `item_decoder()` (concrete
  `JsonDecoder`), `dec.decode<T>()`, `take_ok` the payload, `push` it.
  No bound on `T` is expressible today (`TypeDecoder<T> : Deserializer<T>` is
  not a bound on `T`); a missing element impl surfaces from `decode<T>` at
  instantiation. Add a targeted diagnostic when detectable.
- Span encode only: a span borrows; decoding cannot produce one.
- Maps: concrete `std::string` key argument (partial target pattern), iterate the
  existing `Iterable` impl (`ordered_map.ch:212`,
  `unordered_map2.ch:234`), `MapEncoder.encode<std::string, V>`.
  Decode via `JsonMapDecoder.item_decoder()` (`JsonStringDecoder` key +
  `JsonDecoder` value). `ordered_map` preserves insertion order; `unordered_map`
  does not.
- Reuse the single JSON escaper; never reimplement it; no panics.

Gate: vector round-trips on TCC + LLVM; nested vectors; destructor safety.

### Phase 7 — `#json` plugin enablement

Files: `lang/libs/json_cbi/src/main.ch`.

- Remove the blanket `BaseTypeKind::Generic` rejection in
  `unsupported_field_reason` (~line 253).
- Generic containers already fall through to `decode<T>` and
  `obj.field<T>(...)`; keep that.
- Add a recursive "obviously unsupported element" check only where it does not
  require demanding impls (pointer/array elements); otherwise let instantiation
  produce the bound error.
- Verify `take_ok`/move semantics for destructible container fields.
- Respect the `#json(Child)` ordering rule in tests.

Gate: §6.3 tests green on TCC + LLVM.

### Phase 8 — Cost/latency validation and docs

- Laziness, dedup, no-whole-interface-graph, no-duplicate-vtable tests.
- Record compile time, peak RSS, binary size before/after; call out > 1.05×.
- Update proposals 0003/0014, `language-goals.md` §6.8, and the
  `generics`/`json_serialization` skills.
- Remove the `FutureHandle<T>` structural workaround only if trivial (separate
  change).

---

## 6. Test matrix

### 6.1 Common core-language tests

`lang/tests/common/src/generic/impls.ch`, called from `run_common_tests()`.

| Test | Pins |
|---|---|
| `impl<T> Describe for Box<T>` on `Box<int>`/`Box<long>` | Per-instantiation methods |
| `impl<A,B> … for Pair<A,B>` | Multiple params |
| `impl<T : Describe> Outer for Box<T>` | Inline bound satisfied |
| `impl<T> Show for Wrapper<Box<T>>` | Nested pattern |
| `impl<V> Show for Pair<Tag, V>` | Concrete pattern arg |
| Call via `<T : Describe>` | Static dispatch |
| `impl<T> core::ops::Add for Num<T>` + `a+b` | Operator demand |
| `impl<T> core::iterable::Linear for Box<T>` + `for` | Iteration demand |
| `Box<int>` used, impl unused, bound unsatisfiable | **Laziness/I3** |
| Destructible element round-trip + drop count | No extra copies/drops |
| Two instantiations in one module | Dedup |
| Same type as `GenericType` and relinked `StructDefinition` | Canonical dedup |
| Interface satisfaction (`dyn`/where) | Typeverify demand |
| Generic method / nested impl / default / inherited / static interface | Coverage |
| Union and variant targets | Coverage |
| One generic impl calling a method from another impl for the same target | Deferred-child path |

### 6.2 Negative tests

`lang/tests/negative/src/generic_impls.ch` (update `inheritance.ch`):

- unused parameter;
- non-generic target;
- non-linear pattern (`Pair<T,T>`);
- argument-count mismatch;
- duplicate identical impls;
- ambiguous impls (lazy, at use);
- unsatisfied bound at the demand site;
- missing interface member / wrong signature on a **generic interface**
  (currently skipped by `verify_interface_implementation`);
- `where` clause on an impl (explicit rejection);
- structural target (`impl<T> Foo for *T`) rejection;
- **laziness/symbol test**:
  - compile a module declaring an unused generic impl → `--emit-c` contains no
    impl method symbol **and no vtable symbol**;
  - compile a version that uses it → symbols present.
- **concrete-vs-generic overlap**: a concrete impl exists → no generic instance
  is emitted.

`neg_impl_generic_params_nested` (`impl <T> I for S<T>` inside `struct S<T>`)
redeclares/shadowes the enclosing `T`. Do **not** convert it to positive without
defining shadowing/scoping semantics; keep it negative in v1. **[review]**

### 6.3 `#json` compiler-plugin tests

Extend `lang/tests/compiler_plugins/json/src/test.ch`:

- `JMVectorInt` exact encode `{"values":[1,2,3]}`, decode, empty `[]`.
- `vector<std::string>` with quotes/backslashes/unicode/empty strings.
- `vector<JMChild>` nested struct element.
- `vector<vector<int>>` recursive container.
- Vector inside a nested struct.
- Large vector (512) round-trip.
- `std::span<int>` encode (if Phase 6 includes it).
- `ordered_map<std::string,int>` round-trip + key order; `unordered_map`
  content (order-agnostic).
- Destructible elements with drop counters / no double free.
- Negative: `vector<NotSerializable>` compile error naming the element type.

### 6.4 Cross-cutting regression tests

- Two/three-module dedup and emission (define/use split).
- Clean vs `--cache` symbol identity (`nm` / `--emit-c`).
- `dyn` over an instantiated impl on TCC, LLVM and interpreter (vtables).
- No duplicate vtable definitions across modules.
- Instantiating one impl does not instantiate unrelated interface graph nodes.
- Threaded `--tcc` build with many files using the same impl (race smoke test).

### 6.5 Commands

```bash
./scripts/test.sh --tcc
./scripts/test.sh --llvm
./scripts/test.sh --tcc --interpret
./scripts/test.sh --tcc --plugins
./scripts/test.sh --llvm --plugins
./scripts/test.sh --tcc --negative
./scripts/test.sh --tcc --libs
```

Focused: `--test-names` / `--test-ids` (see `AGENTS.md`).

---

## 7. Diagnostics catalog

| Situation | Message (shape) |
|---|---|
| Unused parameter | `generic type parameter 'T' is not used by the implemented type` |
| Non-generic target | `generic parameters on an implementation require a generic target type (e.g. 'impl<T> Trait for Box<T>')` |
| Duplicate parameter | `type parameter 'T' is used more than once in the implemented type` |
| Count mismatch | `implemented type 'Pair' expects 2 type arguments, got 1` |
| Structural target | `an implementation target must be a named generic struct, union or variant` |
| Duplicate impl | `duplicate implementation of 'Trait' for 'Pair<T>'` + note at first |
| Ambiguity | `ambiguous generic implementations of 'Trait' for 'Pair<int>'` + notes, deterministic order |
| Unsatisfied bound | `type 'Foo' does not satisfy bound 'Trait' required by 'impl<T : Trait> …'` at demand site |
| Recursion | `generic implementation expansion exceeds the recursion limit` |
| Missing method | existing `unresolved child 'x' in parent 'Type'`; add a note listing candidates where cheap |

Locations must point at the demand site and the impl bound; messages must be
actionable from the error text alone.

---

## 8. Performance / memory validation

Measure and record:

- compile user/wall time and peak RSS for the common suite, with/without unused
  generic impls;
- generated C line count and executable size for 0/1/10/100 instantiations;
- symbol counts: unused ⇒ 0; used ⇒ one per distinct canonical args;
- registry memory for 1000 declared-but-unused impls.

Gate: **≤ 1.05×** compile time/RSS on the existing suite; unused impls add only
registry memory. Use the benchmark dashboard for the long-term trend; the laziness
and dedup tests are the deterministic in-suite guards.

---

## 9. Risks & mitigations

| Risk | Mitigation |
|---|---|
| Eager instantiation sneaks in | Laziness symbol test + review every `register_generic_args` caller |
| `body_linked` lost update / vector race | Status sequence + snapshot under registration mutex (§4.4) |
| Adoption vs lookup data race | Shared-lock lookups on concrete generic containers; recursive adopt mutex |
| Same-thread recursion deadlock | Pending-self rule + recursion diagnostic |
| Dedup representation mismatch | Canonical arg key + concrete-vs-generic tests |
| Generic interface nodes skipped by legacy checks | Shared `InterfaceDecl`/`GenericInterfaceDecl` helpers |
| Deferred child in generic body | Explicit deferred marker + relink retry + test |
| Cache naming drift under lazy registration | Clean-vs-cached symbol identity test |
| Interpreter `dyn` missing vtables | Pass current instantiations to `build_module_vtables` for both backends |
| Half-registered impl after an error | Failed status/removal from current instantiations |
| Visibility leak via job-global registry | Access/module check at demand |

---

## 10. Execution order

1. Baseline (Phase 0).
2. Parser + explicit master-type linkage (Phase 1).
3. Serial registration + registry + matcher + validation (Phase 2).
4. Lazy method demand, status/locking, body environment, dedup (Phase 3).
5. Run core language tests on TCC/LLVM/interpret. **Stop and review.**
6. Operators/iteration/typeverify demand (Phase 4).
7. Codegen/interpreter/cache (Phase 5).
8. Containers (Phase 6) and `#json` (Phase 7).
9. Plugin tests + perf/laziness gates + docs (Phase 8).

Each phase leaves the touched suites green; no phase relies on a later phase to
fix a regression.

---

## 11. Decisions requested before coding

1. **Lazy demand-driven instantiation** (recommended) vs eager. Lazy is required
   for "cost only when used" and correct bound errors.
2. **v1 container scope:** `vector` (encode+decode) + `span` (encode) + maps with
   `std::string` keys (encode+decode). Confirm maps now or as a follow-up.
3. **Partial target patterns** (`ordered_map<std::string, V>`) are needed for
   string-keyed maps without a new trait. Confirm concrete pattern arguments are
   allowed (non-linear patterns still rejected).
4. **Inline bounds only** (`impl<T : A + B>`); `where` on impl headers is
   explicitly rejected in v1. Confirm.
5. **Coherence/diagnostics:** identical patterns rejected at registration;
   overlapping patterns diagnosed lazily at use, deterministically. Confirm.
6. **Nested `impl<T> … for S<T>` inside `struct S<T>`** keeps its current
   rejection (parameter shadowing is undefined). Confirm.
7. **Link-signature comptime demand** across files is a documented limitation
   (no pending queue in v1). Confirm.
