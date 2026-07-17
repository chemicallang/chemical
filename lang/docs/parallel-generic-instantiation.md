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

## The GenericInstantiationPass Optimization Opportunity

### What GenericInstantiationPass Does

The `GenericInstantiationPass` (`compiler/symres/GenericInstantiationPass.cpp`) runs after `link_signature` and before `link_body`. It:

1. Visits `GenericType` nodes in **signatures only** (not bodies)
2. Calls `type->instantiate(generic_instantiator, type_location)` which calls `register_generic_args`
3. `register_generic_args` locks mutex → registers → **finalizes signature** (under lock) → unlocks → finalizes body

The key observation: **the GenericInstantiationPass only needs the pointer** to attach to the `GenericType` node. It doesn't need the instantiated type's signature to be finalized.

### Why Signature Finalization Is Unnecessary Here

When the GenericInstantiationPass encounters `func check(v : vec2<int>)`:

1. It needs to know that `vec2<int>` exists → **registration needed**
2. It needs to set `linked_ptr = vec2<int> impl` on the `GenericType` node → **pointer needed**
3. It does NOT need to know what methods `vec2<int>` has, what their parameter types are, or what they return → **signature finalization NOT needed**

Signature finalization replaces generic type parameters with concrete types in the instantiated type's own fields, methods, etc. This is only needed when:
- The **link body pass** resolves method calls (e.g., `v.push(42)` needs to know `push`'s parameter type)
- Another type references this type's methods in its body

In the GenericInstantiationPass, we're only visiting signatures. We never traverse into the instantiated type's methods.

### The Serialization Problem

Currently, `register_generic_args` holds the mutex through signature finalization:

```
Thread A: processing file1, encounters std::vector<int>
  1. Lock reg_mutex
  2. Register std::vector<int> (new)
  3. Finalize signature of std::vector<int>  ← HOLDING LOCK (~ms)
  4. Unlock reg_mutex

Thread B: processing file2, encounters std::vector<long>
  1. Lock reg_mutex  ← BLOCKS until Thread A finishes signature finalization
  2. Register std::vector<long> (new)
  3. Finalize signature of std::vector<long>
  4. Unlock reg_mutex
```

Thread B blocks for the entire duration of Thread A's signature finalization, even though `std::vector<int>` and `std::vector<long>` are completely independent instantiations.

### Potential Solution: Unlock Before Signature Finalization

Add a mode/flag to `register_generic_args` that controls whether to hold the lock through signature finalization:

```
register_generic_args(mode = RegisterOnly):  ← used by GenericInstantiationPass
  1. Lock reg_mutex
  2. register_generic_usage → (index, is_new)
  3. If !is_new: unlock, return existing pointer
  4. If is_new:
     a. Shallow copy master_impl
     b. Push to instantiations vector
     c. Unlock reg_mutex  ← IMMEDIATELY after registration
     d. Finalize signature (NO lock)
     e. Finalize body (NO lock)
     f. Return impl

register_generic_args(mode = Full):  ← used by link body pass
  1. Lock reg_mutex
  2. register_generic_usage → (index, is_new)
  3. If !is_new: unlock, return existing pointer
  4. If is_new:
     a. Shallow copy master_impl
     b. Push to instantiations vector
     c. Finalize signature (STILL HOLDING LOCK)
     d. Unlock reg_mutex
     e. Finalize body (NO lock)
     f. Return impl
```

With this approach:

```
Thread A: processing file1, encounters std::vector<int>
  1. Lock reg_mutex
  2. Register std::vector<int> (new)
  3. Unlock reg_mutex  ← IMMEDIATELY
  4. Finalize signature of std::vector<int> (NO lock)

Thread B: processing file2, encounters std::vector<long>
  1. Lock reg_mutex  ← CAN PROCEED IMMEDIATELY (Thread A unlocked)
  2. Register std::vector<long> (new)
  3. Unlock reg_mutex  ← IMMEDIATELY
  4. Finalize signature of std::vector<long> (NO lock)

→ Both signature finalizations happen in parallel!
```

### Safety Analysis for RegisterOnly Mode

**Question**: When Thread A unlocks after registration and Thread B registers a different instantiation, Thread B might encounter Thread A's instantiation during signature finalization. The signature isn't finalized yet. Is this safe?

**Answer**: YES, for the GenericInstantiationPass:

1. We only need the **type pointer** — the struct definition is valid (allocated during registration)
2. We set `linked_ptr` on `GenericType` nodes — simple pointer assignment
3. We don't call methods on the type or walk its members at this point
4. The signature will be finalized later (in the link body pass or when `register_generic_args` is called with `Full` mode)

**But**: In the link body pass, we DO need finalized signatures. So the link body pass must use `Full` mode.

### Interaction with Circular Dependencies

The recursive mutex still handles circular dependencies within a single thread:

```
Thread A: vec1<int>::register_generic_args(RegisterOnly)
  1. Lock reg_mutex (count=1)
  2. Register vec1<int> (new)
  3. Unlock reg_mutex (count=0)
  4. Finalize signature of vec1<int> (NO lock)
     → encounters vec2<int> → calls vec2<int>::register_generic_args(RegisterOnly)
       5. Lock reg_mutex (count=1)
       6. Register vec2<int> (new)
       7. Unlock reg_mutex (count=0)
       8. Finalize signature of vec2<int> (NO lock)
          → encounters vec1<int> → already registered, returns pointer
       9. Return vec2<int> impl
     → vec1<int> signature continues with vec2<int> pointer
  10. Return vec1<int> impl
```

No deadlock, no waiting. Both signatures finalize in parallel (on the same thread, sequentially, but without holding the lock).

## Exploring Condvar Approach for Independent Instantiations

### The Core Problem

Currently, a single `reg_mutex` serializes ALL instantiations of ALL generics. When Thread A is registering `vector<int>` and Thread B tries to register `vector<long>`, Thread B blocks until Thread A finishes signature finalization. But `vector<int>` and `vector<long>` are completely independent — they should be able to proceed in parallel.

### The Key Insight: What Each Pass Actually Needs

When `register_generic_args` encounters another generic type during its work, the **requirement depends on which phase we're in**:

**During signature finalization** (e.g., `vec1<int>` encounters `vec2<T>` in `func check(v : vec2<T>)`):
- We only need the **pointer** to `vec2<int>` so we can set `linked_ptr` on the `GenericType` node
- We do NOT need `vec2<int>`'s method signatures, parameter types, or return types
- We do NOT need `vec2<int>`'s body to be finalized
- **Only registration is required**

**During body finalization** (e.g., `vec1<int>` calls `v.push(42)` where `v : vec2<int>`):
- We need to know `vec2<int>.push`'s parameter type (is it `int`? `long`? `float`?)
- We need to resolve method calls, determine concrete types
- **Signature finalization of `vec2<int>` is required**

This asymmetry is the key to solving the parallelism problem.

### The Solution: Phase-Aware Synchronization

```
register_generic_args:
  1. Lock reg_mutex
  2. Register → (index, is_new)
  3. If !is_new: unlock, return pointer
  4. If is_new:
     a. Set status = Building, builder_thread = current_thread
     b. Shallow copy, push to vector
     c. Unlock reg_mutex

     // SIGNATURE FINALIZATION (no reg_mutex held)
     d. Finalize signature
        - When encountering generic type → call register_generic_args
          → only needs registration → no waiting, return pointer

     // Mark signature as finalized
     e. Lock inst_status_mutex
     f. Set status = SignatureFinalized
     g. Notify condvar
     h. Unlock inst_status_mutex

     // BODY FINALIZATION (no reg_mutex held)
     i. Finalize body
        - When encountering generic type → call register_generic_args
          → needs signature finalized
          - If status == Building && builder_thread == current_thread:
              return pointer (no waiting — we're building it ourselves)
          - If status == Building && builder_thread != current_thread:
              wait on condvar until SignatureFinalized
          - If status == SignatureFinalized:
              return pointer

     j. Return impl
```

### How This Avoids ABBA Deadlock

The ABBA deadlock requires two threads **waiting for each other simultaneously**. This solution avoids it because:

1. **Signature finalization never waits**: When encountering a Building type during signature finalization, we just return the pointer. No condvar, no waiting.

2. **Body finalization only waits for signatures, not bodies**: When encountering a Building type during body finalization, we wait for its **signature** to be finalized — not for its body. Since signatures are finalized before bodies (in the same thread), and signature finalization never waits, there's no circular waiting.

3. **Same-thread detection**: If we're building the dependency ourselves (same thread), we don't wait — we know its signature will be finalized soon.

### Trace: Circular Dependencies (Same Thread)

```
Thread A: vec1<int>::register_generic_args()
  1. Register vec1<int> (new), builder_thread = Thread A
  2. Unlock reg_mutex
  3. Finalize signature of vec1<int> (no lock)
     → encounters vec2<int> → calls vec2<int>::register_generic_args()
       4. Register vec2<int> (new), builder_thread = Thread A
       5. Unlock reg_mutex
       6. Finalize signature of vec2<int> (no lock)
          → encounters vec1<int> → already registered, returns pointer
       7. Set vec2<int> status = SignatureFinalized, notify condvar
       8. Finalize body of vec2<int> (no lock)
          → encounters vec1<int> → status = Building, builder_thread = Thread A (same thread)
          → returns pointer (NO WAITING)
       9. Return vec2<int> impl
  10. Set vec1<int> status = SignatureFinalized, notify condvar
  11. Finalize body of vec1<int> (no lock)
      → encounters vec2<int> → status = SignatureFinalized, returns pointer
  12. Return vec1<int> impl
```

No deadlock. Both types are built by the same thread. During body finalization of `vec2<int>`, `vec1<int>` is Building but by the same thread — no waiting.

### Trace: Parallel Instantiations (Different Threads)

```
Thread A: processing file1, encounters std::vector<int>
  1. Register std::vector<int> (new), builder_thread = Thread A
  2. Unlock reg_mutex
  3. Finalize signature (no lock)

Thread B: processing file2, encounters std::vector<long>
  4. Register std::vector<long> (new), builder_thread = Thread B
  5. Unlock reg_mutex  ← CAN PROCEED (Thread A unlocked at step 2)
  6. Finalize signature (no lock)  ← IN PARALLEL with Thread A

Thread A: std::vector<int> signature encounters std::vector<long>
  7. Calls register_generic_args for std::vector<long>
     → already registered by Thread B, returns pointer (no waiting)

Thread B: std::vector<long> signature encounters std::vector<int>
  8. Calls register_generic_args for std::vector<int>
     → already registered by Thread A, returns pointer (no waiting)

Thread A: signature done → set SignatureFinalized → body finalization
Thread B: signature done → set SignatureFinalized → body finalization
  → Both body finalizations happen in parallel
  → If they encounter each other, signatures are already finalized → no waiting
```

`std::vector<int>` doesn't block `std::vector<long>`. They proceed in parallel.

### Trace: Cross-Thread Body Finalization

```
Thread A: body finalization of std::vector<int>
  → encounters std::vector<long>
  → status = SignatureFinalized (Thread B already finished signature)
  → returns pointer (no waiting)

Thread B: body finalization of std::vector<long>
  → encounters std::vector<int>
  → status = SignatureFinalized (Thread A already finished signature)
  → returns pointer (no waiting)
```

Even if Thread B is slower and still in signature finalization when Thread A starts body finalization:

```
Thread A: body finalization of std::vector<int>
  → encounters std::vector<long>
  → status = Building, builder_thread = Thread B (different thread)
  → waits on condvar for std::vector<long>

Thread B: signature finalization of std::vector<long> completes
  → set status = SignatureFinalized
  → notify condvar
  → Thread A wakes up, continues

Thread B: body finalization of std::vector<long>
  → encounters std::vector<int>
  → status = SignatureFinalized (Thread A already finished signature)
  → returns pointer (no waiting)
```

No ABBA deadlock. Thread A waits for Thread B's **signature** (not body). Thread B doesn't wait for Thread A because Thread A's signature is already finalized.

### Why This Is Better Than the Old Condvar Approach

The old approach had two problems:

1. **ABBA deadlock**: Thread A building `vec1<int>` waited for Thread B building `vec2<int>`, which waited for Thread A. Both waited for each other's **body** finalization.

2. **`activateIteration` reading incomplete data**: The nesting depth approach returned pointers before the instantiation was fully set up.

This solution avoids both:

1. **No ABBA deadlock**: Signature finalization never waits. Body finalization only waits for signatures (not bodies). Since signatures are finalized before bodies, there's no circular waiting.

2. **`activateIteration` is safe**: `register_generic_usage` is called BEFORE any finalization, so the types are always set in the container when the pointer is returned. The `activateIteration` method reads types from the container, which are set during registration.

### Data Structures Needed

```cpp
// On BaseGenericDecl:
enum class InstantiationStatus { Building, SignatureFinalized };
struct InstantiationStatusEntry {
    InstantiationStatus status;
    std::thread::id builder_thread;
    std::condition_variable cv;
};
std::vector<InstantiationStatusEntry> instantiation_statuses;

// On InstantiationsContainer:
std::mutex inst_status_mutex;  // protects status changes
```

### Concurrency Guarantees

| Phase | Mutex Held? | What It Waits For |
|-------|-------------|-------------------|
| Registration | `reg_mutex` | Nothing (serialized) |
| Signature finalization | None | Nothing (never waits) |
| Mark SignatureFinalized | `inst_status_mutex` | Nothing (brief lock) |
| Body finalization | None | Dependency's signature (via condvar) |
| Same-thread dependency | None | Nothing (skip wait) |

### Summary

| Approach | ABBA Deadlock? | Parallelism | Complexity |
|----------|---------------|-------------|------------|
| Single mutex (current) | No | Serial (all generics) | Low |
| Old condvar (removed) | Yes | Parallel | Medium |
| **Phase-aware condvar (proposed)** | **No** | **Parallel (independent generics)** | **Medium** |

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
