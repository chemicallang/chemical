# Safety Assessment: Essential Features for Scalability

## Problem Statement

Chemical needs a careful balance: the language is designed for flexibility and ergonomic syntax (people won't adopt it if it's overly restrictive), but without certain critical safety features, scalability is fundamentally threatened. This document identifies the minimum set of safety mechanisms required before Chemical can be used for large, multi-developer codebases.

## Current Safety Posture

Chemical already has several safety mechanisms:

| Mechanism | Status | Location |
|-----------|--------|----------|
| Destructor-based move semantics (drop flag clearing) | Implemented | `LLVM.cpp:2152` |
| Use-after-move tracking | Implemented | `FunctionType.cpp:441` |
| Scope-aware move tracking (if/else branches) | Implemented | `SymResLinkBody.cpp:75` |
| Deref prevention for destructible structs | Implemented | `SymResLinkBody.cpp:2764` |
| Zero-init prevention for types with ctors/dtors | Implemented | `SymResLinkBody.cpp:3338` |
| Top-level destructible variable restriction | Implemented | `TypeVerify.cpp:261` |
| Top-level `@never_destructed` annotation | Implemented | `AnnotationController.cpp:278` |

These form a good foundation. The gaps below are the ones that will cause the most pain as codebases grow.

## Critical Safety Gaps

### 1. No Trait/Constraint System for Generics

**Severity: Critical**

Generic containers (`vector<T>`, `unordered_map<K,V>`) have no way to express requirements on their type parameters. This means:

- `vector<T>.resize()` uses `zeroed<T>()` for new elements, which is invalid for types with constructors or destructors (acknowledged via TODO in `vector.ch:50-51`)
- `vector<T>.push()` uses `memcpy` + `intrinsics::forget`, assuming T is trivially relocatable — no compile-time verification
- Any function accepting a generic type parameter cannot constrain whether it needs destructibility, copyability, or default-constructibility

**What is needed:** A minimal trait system (interfaces/constraints) that allows:

```
struct vector<T: TriviallyRelocatable + Destructible> { ... }
```

At minimum, the compiler should provide built-in traits like:
- `Destructible` — has a destructor
- `TriviallyRelocatable` — safe to memcpy + forget
- `DefaultConstructible` — has a default constructor or is zeroable
- `Copyable` — can be safely memcpy'd (no destructor)

Without this, generic code is inherently unsound when used with the wrong type.

### 2. Container `get` Returns Value Without Ownership Transfer

**Severity: Critical**

```chemical
// vector.ch:96
func get(&self, index : size_t) : T {
    return data_ptr[index];
}
```

When `T` is destructible, `data_ptr[index]` performs a memcpy of the element out of the vector. The original element in the vector remains intact (its destructor hasn't been suppressed). When the vector is destroyed, it destructs all its elements — including this one. The caller also destructs their copy. **Double-free.**

**Root cause:** The compiler currently only prevents dereferencing a pointer to a destructible struct (`SymResLinkBody.cpp:2764`) in certain contexts. The `data_ptr[index]` path in `get` is not adequately guarded because of how array subscript dereference interacts with return value semantics.

**What is needed:** One of:
- Make `get` return a reference (`&T`) instead of `T` by value when T is destructible
- Require an explicit `.clone()` / `.copy()` when extracting by value from a container
- Make `get` only available for non-destructible types (via the trait system above)
- Automatically suppress destructor on the source after memcpy (move semantics applied to container internals)

The last option is the most flexible: when `data_ptr[index]` is used as a return value or assignment, and T is destructible, the compiler should treat it as a **move** from the container element — setting the drop flag on the container's element to false. However, this would leave a hole in the container (zombie element). The cleanest solution is to restrict `get` to non-destructible types and use `get_ptr`/`get_ref` for destructible types.

### 3. No Borrow Checker / Reference Lifetime Tracking

**Severity: Critical for scalability**

Currently, references (`&T`, `&mut T`) have no lifetime tracking. The existing SafetyChecklist.md lists this but there's no implementation. Without this:

- Returning a reference to a local variable compiles but creates a dangling reference
- Storing a reference in a struct that outlives the referent is allowed
- Multiple mutable references can coexist (data races in multithreaded code)

**What is needed:** Minimum viable borrow checking:
- References cannot outlive their referent (checked at return and assignment)
- Either one mutable or many immutable references (alias-based, following existing borrow checker patterns)
- References stored in structs must have lifetimes tied to the struct's scope

This is the hardest feature to add without disrupting ergonomics, but it is essential before Chemical can be used for anything beyond single-file scripts.

### 4. No Explicit Clone/Copy Distinction

**Severity: High**

For non-destructor types, Chemical implicitly memcpys on assignment and parameter passing (like C). For destructor types, it implicitly moves. This works well most of the time, but there is no way to explicitly say "I want to clone this value" for destructible types.

```chemical
var a = ManagedResource{...}  // has destructor
var b = a  // implicit move, a is now invalid

// How to make a copy?
// Currently no standard mechanism
```

**What is needed:** A `clone` interface/trait that destructible types can implement, and a compiler-enforced rule that destructible types must be explicitly cloned (via a method) when a copy is needed. The implicit rule becomes:
- Non-destructible: implicit memcpy (safe)
- Destructible: implicit move (already implemented), explicit clone required for copying

This is already partially in place (move tracking works), but the clone mechanism and the enforcement that you cannot accidentally copy a destructible type need to be formalized.

### 5. No TriviallyRelocatable Verification

**Severity: High**

The standard library heavily relies on `realloc` + `memcpy` for container growth (`vector.ch:63`). This is only valid for types where memcpy + forget of the old location is a correct move operation. For types with internal self-references (e.g., an intrusive linked list node, or a struct with an internal pointer), `realloc` invalidates those pointers.

**What is needed:** Either:
- A `TriviallyRelocatable` built-in trait that types must satisfy to be used in containers that realloc/memcpy
- Or a guarantee that realloc-based containers only accept trivially relocatable types (checked at compile time)

### 6. Unchecked `@unsafe` Functions

**Severity: High**

The `@unsafe` annotation exists but there is no enforcement that `unsafe` blocks are only used within `@unsafe` functions, or that calling `@unsafe` functions requires `unsafe` context.

**What is needed:**
- `@unsafe` functions can only be called from within `unsafe` blocks
- `unsafe` blocks are only allowed inside `@unsafe` functions
- All operations that could violate memory safety (dereferencing raw pointers, calling dealloc, calling extern C functions) should require `unsafe`

This is a standard safety boundary that Chemical currently lacks.

## Lower Priority but Important

### 7. Integer Overflow Safety

While not as critical as memory safety, integer overflow bugs are a major source of vulnerabilities in large codebases. Chemical should have:
- Wrapping arithmetic by default (as in Rust)
- Explicit overflow-checked operations (`checked_add`, `saturating_add`, etc.)
- Or at minimum, compile-time warnings for operations that could overflow

### 8. Uninitialized Memory Safety

`malloc` returns uninitialized memory. Using it without initialization is undefined behavior. The `resize` / `reserve` functions in vector currently use `zeroed<T>()` which is invalid for non-trivial types. A safer approach would be:
- `make_with_capacity` should construct elements in-place rather than zeroing
- Or require `@allow_zeroed` for types that can be safely zero-initialized

## Implementation Strategy: Gradual Migration

Rather than implementing all of these at once (which would break existing code), the migration should be:

### Phase 1: Trait system for generics (no breakage)
- Add built-in traits: `Destructible`, `TriviallyRelocatable`, `DefaultConstructible`
- These are auto-implemented by the compiler based on struct analysis
- Generic containers add constraint annotations (backward-compatible: unconstrained params imply "any")

### Phase 2: Clone/copy distinction (warnings, then errors)
- Add `Clone` trait with a `clone` method
- Phase 1: warning when a destructible type is implicitly memcpy'd
- Phase 2: error (must use explicit `.clone()`)

### Phase 3: Fix container `get` (targeted fix)
- Add overload or constraint so `get` only works for non-destructible types
- Users migrate to `get_ref` or `get_ptr` for destructible types

### Phase 4: Minimum viable borrow checking (new feature)
- Start with the existing SafetyChecklist.md items
- Non-goal: full Rust-level borrow checker
- Goal: prevent dangling references and iterator invalidation

### Phase 5: `unsafe` boundary enforcement
- Warnings first, then errors
- This is primarily additive (marking existing functions as `@unsafe`)

## Design Principles

1. **Sound by default, but opt-in to unsound operations via `unsafe`** — the safe path should always be correct
2. **No hidden costs** — safety features should not introduce runtime overhead in the safe path (e.g., borrow checking is compile-time only, zero-cost)
3. **Progressive disclosure** — the language should be usable without knowing about traits, borrow checking, or clone semantics. Simple programs "just work." Complex programs get safety guarantees.
4. **C-like where safe** — for non-destructor, trivially-relocatable types, the C memcpy model is fine. Safety restrictions only kick in when destructors or complex ownership are involved.

## Concrete Example: Fixing Vector `get`

The most pressing issue. Current code:

```chemical
func get(&self, index : size_t) : T {
    return data_ptr[index];  // double-free for destructible T
}
```

Proposed fix (requires trait system):

```chemical
// For non-destructible types: by-value access (memcpy, safe)
func get(&self, index : size_t) : T
    where T: ~Destructible  // T must NOT have a destructor
{
    return data_ptr[index];
}

// For destructible types: reference access only (no copy)
func get_ref(&self, index : size_t) : &T {
    return data_ptr[index];
}

func get_mut(&mut self, index : size_t) : &mut T {
    return data_ptr[index];
}
```

The `~Destructible` negative constraint says "T must not have a destructor." This is a limited form of negative trait bounds, which while uncommon, is exactly what's needed here. An alternative is to simply omit the by-value `get` entirely and always use `get_ref` / `get_mut` / `get_ptr` — but this sacrifices ergonomics for non-destructible types (which are the common case for numeric data).

## Summary

| Feature | Severity | Current Status | Effort |
|---------|----------|---------------|--------|
| Trait/constraint system for generics | Critical | Not implemented | Large |
| Container get returns value unsafely | Critical | Known bug | Medium |
| Borrow checker / lifetime tracking | Critical | Checklist only | Very large |
| Explicit clone/copy for destructible types | High | Partially implemented | Medium |
| TriviallyRelocatable verification | High | Not implemented | Medium |
| `@unsafe` boundary enforcement | High | Annotations exist, no checks | Medium |
| Integer overflow safety | Medium | Not implemented | Small |
| Uninitialized memory safety | Medium | Partial (zeroed/TODO) | Medium |
