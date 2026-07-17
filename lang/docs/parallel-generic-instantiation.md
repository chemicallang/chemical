# Parallel Generic Instantiation

## Overview

The Chemical compiler instantiates generic types and functions (e.g., `std::vector<int>`, `unordered_map<string, JsonValue>`) during compilation. This document explains how the instantiation system works, how parallelism is achieved, the concurrency problems encountered, and the solutions implemented.

## How Generic Instantiation Works

### Generic Declarations

A generic declaration defines a blueprint:

```chemical
struct Vec<T> {
    var a : T
    func give() : Vec<T> {
        return create_vec<T>()
    }
}

func <T> create_vec() : Vec<T> {
    return Vec<T> { a: 0 }
}
```

When the user writes `Vec<int>`, the compiler must **instantiate** it: create a concrete copy of `Vec` where `T` is replaced with `int`. This involves:

1. **Shallow copy** the master implementation (`Vec<T>`) into a new `StructDefinition`
2. **Register** the instantiation in the `InstantiationsContainer`
3. **Finalize signature**: replace generic type parameters with concrete types in parameter types, return types, variable types, inherited types
4. **Finalize body**: replace generic type parameters in function bodies, resolve method calls, relink identifiers

### Registration

Registration (via `register_generic_usage`) ensures that each unique combination of generic declaration + concrete type arguments is only instantiated once. For example, `Vec<int>` is registered once; subsequent encounters reuse the same pointer.

The registration check is protected by `registration_mutex` (`reg_mutex`). This mutex serializes the check-and-insert operation so that two threads don't both create the same instantiation.

### The `register_generic_args` Pattern

Each `Generic*Decl` (GenericStructDecl, GenericFuncDecl, GenericUnionDecl, GenericInterfaceDecl, GenericVariantDecl, GenericImplDecl, GenericTypeDecl) has a `register_generic_args` method that:

```
1. Lock reg_mutex
2. Call register_generic_usage → (index, is_new)
3. If !is_new (already registered):
      Unlock reg_mutex
      Return the existing pointer
4. If is_new (new registration):
      Shallow copy master_impl
      Set generic_parent, generic_instantiation
      Push to instantiations vector
      Finalize signature (replace T → int in signatures)
      Unlock reg_mutex
      Finalize body (replace T → int in bodies)
      Return new impl
```

### Pass Structure

Generic instantiation happens in two passes within `sym_res_module_par`:

1. **Pass 1 (`link_body_generic_decls`)**: Processes all `Generic*Decl` nodes. Registers instantiations and finalizes signatures. This ensures all generic types used in signatures are available.

2. **Pass 2 (`link_body`)**: Processes the full file body. When encountering generic types in non-generic code (e.g., `var v = Vec<int>()`), calls `register_generic_args` which reuses already-registered instantiations.

## The Parallelism Model

### File-Level Parallelism

Each source file is processed by a separate thread from the thread pool:

```cpp
for(auto& file_ptr : module->direct_files) {
    futures.emplace_back(pool.push([this, &file](int id){
        auto res = link_body_task(resolver, &file);
        return res.has_errors;
    }));
}
```

Threads share the `InstantiationsContainer` and `reg_mutex`. The container maps generic declarations to their instantiation lists.

### Intra-File Sequential, Inter-File Parallel

Within a single file, instantiation is sequential (one thread). Between files, instantiation is parallel (multiple threads). This means:

- Two files can simultaneously instantiate different generic types
- Two files can simultaneously try to instantiate the **same** generic type — `reg_mutex` serializes this
- During body finalization, a thread may encounter generic types from other files

## The Deadlock Problem

### Setup: Circular Dependency

Consider two mutually-dependent generic types:

```chemical
struct vec1<T> {
    func check(v : vec2<T>) {}
}

struct vec2<T> {
    func check(t : vec1<T>) {}
}
```

When instantiating `vec1<int>`, the signature finalization of `vec1<int>` encounters `vec2<int>` in the `check` method's parameter type. Finalizing `vec2<int>`'s signature then encounters `vec1<int>` back.

### Problem 1: Recursive Lock (Non-Recursive Mutex)

With a plain `std::mutex`, the same thread holds the lock and tries to re-acquire it:

```
Thread A: vec1<int>::register_generic_args()
  1. Lock reg_mutex
  2. Register vec1<int> (new)
  3. Finalize signature of vec1<int>  ← STILL HOLDING LOCK
     → encounters vec2<int> in func check(v : vec2<T>)
     → calls vec2<int>::register_generic_args()
       4. Lock reg_mutex  ← DEADLOCK (same thread, non-recursive mutex)
```

With `std::mutex`, this is undefined behavior (typically deadlock on Linux). This is why we use `std::recursive_mutex` — the same thread can re-enter the lock.

### Problem 2: Cross-Thread ABBA Deadlock (Condvar Waiting)

Before the current solution, a condvar-based approach was used where threads would wait for in-progress instantiations to finalize. This introduced a classic ABBA deadlock:

```
Thread A: building vec1<int>
  1. Register vec1<int> (status = Building)
  2. Finalize signature → encounters vec2<int>
  3. Finds vec2<int> status = Building (by Thread B)
  4. Waits on condvar for Thread B to finish

Thread B: building vec2<int>
  1. Register vec2<int> (status = Building)
  2. Finalize signature → encounters vec1<int>
  3. Finds vec1<int> status = Building (by Thread A)
  4. Waits on condvar for Thread A to finish

→ Deadlock: both threads wait for each other
```

A thread-local nesting depth counter was tried to skip the wait when already inside a `register_generic_args` call. This broke `activateIteration` because it reads types from the container expecting them to be fully set up — but the skipped-wait path returned an incomplete instantiation.

## The Solution: Recursive Mutex + Atomic Registration + Signature

### Key Insight

During signature finalization, we only need the **pointer** to an already-registered instantiation — we don't need to wait for its finalization. The pointer is safe to use as a type reference because:

1. The struct definition exists and is valid (allocated during registration)
2. We only set `linked_ptr` on `GenericType` nodes — we don't walk the type's methods
3. The signature will be fully finalized by the time we return from the outer `register_generic_args`

### How Circular Dependencies Resolve

With `std::recursive_mutex`, the same thread can re-enter the lock. The circular case resolves naturally:

```
Thread A: vec1<int>::register_generic_args()
  1. Lock reg_mutex (count=1)
  2. Register vec1<int> (new)
  3. Finalize signature of vec1<int>  ← STILL HOLDING LOCK
     → encounters vec2<int> in func check(v : vec2<T>)
     → calls vec2<int>::register_generic_args()
       4. Lock reg_mutex (count=2, recursive, same thread)
       5. Register vec2<int> (new)
       6. Finalize signature of vec2<int>  ← STILL HOLDING LOCK
          → encounters vec1<int> in func check(t : vec1<T>)
          → calls vec1<int>::register_generic_args()
            7. Lock reg_mutex (count=3, recursive, same thread)
            8. register_generic_usage → vec1<int> ALREADY REGISTERED (itr.second == false)
            9. Unlock reg_mutex (count=2)
            10. Return existing vec1<int> pointer ← NO WAITING, IMMEDIATE RETURN
          → vec2<int> signature finalization continues with vec1<int> pointer
       11. Unlock reg_mutex (count=1)
       12. Return vec2<int> impl
     → vec1<int> signature finalization continues with vec2<int> pointer
  13. Unlock reg_mutex (count=0)
  14. Body finalization of vec1<int> (NO LOCK)
  15. Body finalization of vec2<int> (NO LOCK)
  16. Return vec1<int> impl
```

### Why This Is Safe

The critical question: when `vec2<int>`'s signature finalization encounters `vec1<int>` (step 10), `vec1<int>`'s signature is **partially finalized** (we're in the middle of step 3). Is this safe?

**Yes**, because:
- We only need the **type pointer** — the struct definition is valid (allocated during step 2)
- We set `linked_ptr` on the `GenericType` node in `vec2<int>` — this is a simple pointer assignment
- We don't call methods on `vec1<int>` or walk its members at this point
- `vec1<int>`'s signature will be fully finalized when we return from step 3 (after `vec2<int>` completes)

### Cross-Thread Scenario

The solution also handles the cross-thread case. When two threads try to register different generic types that depend on each other:

```
Thread A: vec1<int>::register_generic_args()
  1. Lock reg_mutex
  2. Register vec1<int> (new)
  3. Finalize signature → encounters vec2<int>
  4. Lock reg_mutex (recursive, same thread)
  5. Register vec2<int> (new)
  6. Finalize signature → encounters vec1<int>
  7. Already registered → return pointer (no waiting)
  8. vec2<int> signature done → unlock (count=1)
  9. vec1<int> signature done → unlock (count=0)

Thread B: vec2<int>::register_generic_args() (arrives after Thread A releases lock)
  1. Lock reg_mutex
  2. register_generic_usage → vec2<int> ALREADY REGISTERED (itr.second == false)
  3. Unlock reg_mutex
  4. Return existing vec2<int> pointer ← NO WAITING
```

Thread B never blocks waiting for Thread A. It finds the registration already done and returns immediately.

### The Updated `register_generic_args` Pattern

```
1. Lock reg_mutex (recursive)
2. Call register_generic_usage → (index, is_new)
3. If !is_new (already registered):
      Unlock reg_mutex
      Return the existing pointer ← NO WAITING, IMMEDIATE RETURN
4. If is_new (new registration):
      Shallow copy master_impl
      Set generic_parent, generic_instantiation
      Push to instantiations vector
      Finalize signature (STILL HOLDING LOCK)
      Unlock reg_mutex
      Finalize body (NO LOCK)
      Return new impl
```

### Pass 1 Optimization

The first pass (`link_body_generic_decls`) only needs registrations, not full finalization. Since registration + signature finalization are now atomic, the pass 1 work is slightly more than needed (it finalizes signatures that might not be needed yet). This is acceptable because:
- Signature finalization is fast (just type replacement)
- The extra work is bounded by the number of registrations
- The benefit is correctness: signatures are always complete when returned

### Concurrency Guarantees

| Operation | Mutex Held? | Thread Safety |
|-----------|-------------|---------------|
| Registration | `reg_mutex` | Serialized |
| Signature finalization | `reg_mutex` | Serialized (part of registration) |
| Body finalization | None | Parallel |
| Reuse of existing pointer | None | Safe — no waiting, immediate return |
| Cross-thread encounter | None | Safe — finds existing registration, returns pointer |

### Why Not Just `std::mutex`?

During signature finalization, we encounter a **new** generic type that needs registration. This calls `register_generic_args` recursively, which tries to lock `reg_mutex` again. With `std::mutex`, this is undefined behavior — the same thread already holds the lock. A `std::recursive_mutex` allows re-entry from the same thread.

Example with `vec1<T>` and `vec2<T>`:
- `vec1<int>::register_generic_args()` locks `reg_mutex` (count=1)
- During signature finalization, encounters `vec2<int>` → calls `vec2<int>::register_generic_args()`
- `vec2<int>::register_generic_args()` locks `reg_mutex` (count=2, recursive)
- During `vec2<int>`'s signature finalization, encounters `vec1<int>` → already registered, unlocks and returns

The performance difference is negligible: `std::recursive_mutex` adds one thread-ID comparison vs `std::mutex`'s atomic CAS. For a compilation toolchain, this is not in the critical path.

## Data Structures

### `InstantiationsContainer`

Maps each generic declaration (keyed by `void*`) to its list of instantiations:

```
key (BaseGenericDecl*) → DeclInstantiations {
    types: vector<span<BaseType*>>    // the concrete types for each instantiation
    implData: vector<void*&>          // reference to the decl's instantiations vector
    registryPositions: vector<...>    // for file-based cleanup (IDE support)
}
```

### `BaseGenericDecl`

Each generic declaration (`GenericStructDecl`, etc.) owns:
- `generic_params`: list of `GenericTypeParameter*`
- `instantiations`: vector of concrete instantiation pointers (e.g., `vector<StructDefinition*>`)

### `GenericInstantiator`

The visitor that replaces generic types with concrete types. Holds:
- `registration_mutex`: reference to the shared `std::recursive_mutex`
- `container`: reference to the shared `InstantiationsContainer`
- `active_type_map`: maps `GenericTypeParameter*` → `BaseType*` for the current instantiation being finalized

### `GenericInstantiatorAPI`

A thin wrapper around `GenericInstantiator` that provides the public API called by `register_generic_args` methods. Created on the stack; manages ownership of the underlying `GenericInstantiator`.
