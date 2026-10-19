# Generic impls

**Chemical · Proposal 0003**

> Let a generic type satisfy a protocol for _every_ type argument, by monomorphizing
> parameterized implementations like generic functions.

**Status:** Approved · **Area:** Language · generics · [review index](review.md)

## Goal fit

- **R3** portable library ecosystem
- **R0 / TC** no compile or size regression
- **R1/R2** bounds feed safety

## 1. Problem

A parameterized implementation cannot be declared at top level. `impl <T> I for
S<T>` is rejected ("don't declare generic parameters"), and
`impl Future<T> for FutureHandle<T>` does not resolve its methods
(`unresolved child 'poll'`; the async code recognises `FutureHandle<U>`
structurally instead). Only a **nested** impl inside the generic type works.
Consequently generic types cannot participate uniformly in protocols — iterators,
conversions, operators, serialization — and code is duplicated per instantiation.

**Concrete, load-bearing consequence:** `std::vector<T>`,
`std::unordered_map<K,V>` and `std::ordered_map<K,V>` are generic declarations
with no `std::Serializer`/`std::Deserializer` impls, so the `#json` plugin cannot
serialize them. That is proposal 14, and it is blocked on this.

## 2. Goals & non-goals

- **In scope:** top-level `impl<T…> Trait for Type<T…>`; multiple parameters;
  bounds/where-clauses; interface method resolution and vtable generation on the
  concrete instantiation; diagnostics and tests; **no compile-time or peak-memory
  regression**.
- **Out of scope:** full typeclass/dictionary (runtime) dispatch; higher-kinded
  types; negative/coherence reasoning beyond basic overlap detection;
  specialization.

## 3. Proposed approach

1. **Parse & validate.** `GenericImplDecl` already exists. Accept
   `impl<T,…> Trait for Target`; require every declared parameter to occur in the
   target type (else a clear diagnostic). The declared parameters bind the
   target's own parameters (this is what the current rejection is standing in for).
2. **Index.** Register the generic impl under its target's generic declaration in
   `ImplementationsIndex`, keyed by (interface, generic type) with a hash lookup —
   not a linear scan.
3. **Instantiate.** Hook `GenericInstantiationPass`: when `Type<Args…>` is
   instantiated, find matching generic impls and clone each into a concrete
   `ImplDefinition` (substitute the arguments), then finalise its methods.
   Instantiate once per (impl, argument list) and cache; instantiation is already
   parallel-safe.
4. **Resolve.** Interface method lookup and interface-satisfaction/vtable
   computation run on the concrete impl, so generic types satisfy interfaces
   normally.
5. **Bounds.** `impl<T: Bound>` is checked at instantiation; unsatisfied bounds
   produce a diagnostic pointing at the instantiation site.
6. **Performance.** Reuse the existing instantiation machinery; hashed indices; no
   whole-program rescans. Add compiler-throughput benchmarks in CI and treat a
   regression as a failure.

## 4. Alternatives considered

- **Dictionary/typeclass dispatch** (evidence passed at runtime): rejected —
  runtime overhead and complexity, contrary to R0.
- **Keep nested-impl-only**: rejected — cannot express cross-module generic
  protocols; blocks the ecosystem.
- **Macro-driven duplication**: rejected — no static checking, poor diagnostics.

## 5. Phased milestones

1. Parser/AST accept the generic-impl header + validation diagnostics.
2. Implementations index keyed by generic target (hashed).
3. Impl instantiation inside the generic instantiation pass (+ cache).
4. Interface method/vtable resolution on concrete impls.
5. Bounds / where-clauses.
6. Tests: positive + negative + **common core-language tests on interpreter, TCC
   and LLVM**.
7. Performance validation (compile time, peak RSS) with a no-regression gate.

## 6. Acceptance criteria

- A top-level `impl<T> SomeTrait for MyType<T>` compiles, resolves its methods,
  and behaves identically on interpreter, TCC and LLVM.
- A generic container can implement a serializer/deserializer (unblocks 14).
- Negative tests: parameter not used by the target; unsatisfied bound; overlapping
  impls.
- No compile-time or peak-memory regression beyond a small documented budget.
- Common core-language test suite green on all three execution paths.
- The async `FutureHandle<T>` structural workaround can be retired.

## 7. Risks & open questions

- **Monomorphization blow-up:** many instantiations inflate generated code and
  binary size (R0/TC). Need a policy (deduplicate identical instantiations; watch
  size).
- Overlap/coherence rules: how strict, and where reported.
- Interaction with the no-panic effect system (05) and native GC (06).
- Method ambiguity when several generic impls apply.

## 8. Dependencies

Depends on: — · Unblocks: 14 (JSON for generic containers), 12 (ecosystem).

## 9. Evidence & references

- Negative tests `neg_impl_declares_generic_params`,
  `neg_impl_generic_params_nested`; positive
  `impl_inside_generic_struct_allowed` (`lang/tests/negative/src/inheritance.ch`).
- Verified `FutureHandle<T>` limitation in `lang/docs/async-await-design.md`.
- `lang/docs/language-goals.md` §6.8; `lang/docs/proposals/DECISIONS.md`
  (decision + constraints).
