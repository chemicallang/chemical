Sure — here’s a **complete, properly formatted draft proposal document** for your language’s **Zero Initialization** feature. It clearly marks this as an early proposal, includes all points you mentioned, and avoids introducing syntax not yet supported (with clear warnings where placeholders appear).

---

# ⚠️ DRAFT PROPOSAL: Zero Initialization Support (`Zeroable`)

> **⚠️ WARNING: ROUGH DRAFT — EXPECT ERRORS, INCOMPLETE SECTIONS, AND CHANGES.**
>
> This document is an **early, rough-stage proposal** for introducing **zero initialization** into the language.
>
> **Help from AI (ChatGPT) was used** in the preparation of this document to organize and articulate ideas.
>
> The design may change significantly in future revisions.

---

## Overview

This proposal introduces **zero initialization** for user-defined types through the `Zeroable` interface marker.

Zero initialization allows certain types (primarily plain data or pointer-based structs) to be safely initialized to **all bits set to zero**, producing a valid instance in a default state — typically with `null` pointers and zeroed integers.

The system is intended to:

* Improve safety around uninitialized memory.
* Allow developers to mark structs that can safely be bitwise zeroed.
* Provide an operator `zeroed(Type)` for simple, predictable initialization.
* Enforce language-level guarantees that zero-initialized structs behave correctly.

---

## Example

```c

// Zeroable is an interface but it doesn't exist anywhere in code
// its a marker interface, when you inherit an interface 

struct vector : Zeroable {
    // compiler will force you to use nullable pointer type (no syntax for it yet)
    var data : *mut int
    var size : size_t
    var capacity : size_t

    @make
    func make() {
        data = malloc(sizeof(int))
        *data = 33
    }

    @delete
    func delete() {
        // checking data for nullability will be forced
        if (data != null && *data == 33) {
            free(data)
        }
    }
}

func create_vector() {
    // stack allocated vector that is initialized to zero using a memset call is returned
    // the safest, fastest initialization
    var vec = zeroed(vector)
}
```

### Explanation

* `Zeroable` acts as a **marker interface**, used mainly for generics and compile-time constraints.
* The expression `zeroed(vector)` returns a zero-initialized instance of `vector`.

---

## Design Goals

1. **Safety and Predictability**

    * Prevent undefined behavior caused by accessing uninitialized memory.
    * Avoid common pitfalls of C-like languages where zeroing a struct can break invariants.

2. **Simple Syntax**

    * The goal is to make zero initialization syntactically simple and intuitive, like:

      ```cpp
      var point = zeroed(Point)
      ```

      similar to `sizeof()` or other built-in operators.

3. **Compile-time Enforcement**

    * The compiler ensures only types explicitly marked with `Zeroable` are zeroable.
    * This prevents accidental zero-initialization of non-zeroable or unsafe types.

---

## Rules and Constraints

### 1. Zeroable Types Must Be Safe to Zero

When a struct uses `Zeroable`, the compiler enforces rules ensuring that zeroing the memory is safe.

* **No non-nullable references allowed**

    * Because references cannot hold null values, their presence makes zero initialization unsafe.
    * Example:

      ```cpp
      struct Invalid : Zeroable {
          var ref: &int  // ❌ invalid, cannot be zeroed
      }
      ```

* **Pointers inside must be nullable**

    * In the future, pointer types may support explicit nullability.
    * Tentatively:

      ```cpp
      var data: ?*mut int  // ? indicates nullable pointer (syntax not finalized)
      ```
    * ⚠️ *Note:* This syntax is not finalized. A different notation for nullability may be used later.

* **No fields with custom invariants**

    * If a type contains a field that requires a specific initialization pattern (e.g., a handle or file descriptor), it must not be zeroable unless zero represents a valid, safe default state.

---

### 2. Destructor Safety

Users must design destructors (`@delete`) with the assumption that all fields might be zero.

For instance:

```cpp
struct Safe : Zeroable {
    var data: *mut int

    @delete
    func delete() {
        if (data != null) {
            free(data)
        }
    }
}
```

In the earlier example, this was unsafe:

```cpp
@delete
func delete() {
    if (*data == 33) { // ❌ unsafe: *data is invalid if data is null
        free(data)
    }
}
```

The compiler will emit a warning (and may enforce) that destructors of zero initialized types must handle null fields safely.

---

### 3. Generics and the `Zeroable` Interface

To support generic functions and containers, `Zeroable` acts as a **compile-time constraint**.
It allows code like:

```cpp
func make_zeroed<T: Zeroable>() -> T {
    return zeroed(T)
}
```

This ensures only types that explicitly implement `Zeroable` can be used in `zeroed()` contexts.

`Zeroable` itself has **no runtime cost** — it’s purely a compile-time marker.

---

### 4. Compiler Enforcement Behavior

The compiler performs several checks when a type `Zeroable`:

| Check                         | Description                                  | Example                                        |
| ----------------------------- | -------------------------------------------- | ---------------------------------------------- |
| Reference fields              | Disallowed                                   | `&int` inside struct                           |
| Non-nullable pointers         | Disallowed (future nullable syntax required) | `*mut int` without `?`                         |
| Fields without zeroable types | Disallowed                                   | Nested non-zeroable struct                     |
| Custom invariants             | Warning or error                             | Types requiring specific initialization        |
| Unsafe destructor             | Warning or error                             | Accessing uninitialized pointer or dereference |

Additionally, when a struct is `Zeroable`, the compiler may auto-verify:

* Default bit pattern (`0`) results in valid field values.
* No runtime assertions would fail if zeroed.
* The type layout is **Plain Old Data (POD)** or safely represents it.

---

## The `zeroed` Operator

`zeroed(Type)` acts similarly to `sizeof(Type)` — it is **an operator**, not a function.

* It creates a **bitwise zeroed** instance of the given type.
* It can only be used with types that are marked `Zeroable`.
* Attempting to call `zeroed()` on a non-zeroable type results in a compile-time error.

### Example

```cpp
func example() {
    var v = zeroed(vector)  // okay
    var p = zeroed(Point)   // error: Point not marked Zeroable
}
```

### Internal Behavior

`zeroed(T)` is semantically equivalent to:

```cpp
memset(&T_instance, 0, sizeof(T))
```

but with **compile-time safety guarantees**.

---

## Implementation Notes

* The `zeroed` operator emits optimized code equivalent to a compile-time or inline memset.

---

## Future Extensions

1. **Nullable Pointer Types**

    * Planned syntax for nullable pointers (`?*mut T` or other) to make zeroable structs safer.
    * Access to nullable pointers will require explicit null checks or unsafe blocks.

2. **Optional Types**

    * `?T` for optional values (may conflict with nullable syntax; TBD).

3. **Constexpr Zeroing**

    * In the future, allow `zeroed(T)` in compile-time contexts.
---

## Summary

| Concept                  | Description                                                         |
| ------------------------ | ------------------------------------------------------------------- |
| `Zeroable`               | Marker interface for generics and constraints                       |
| `zeroed(Type)`           | Operator returning a bitwise-zeroed instance                        |
| Safety Rules             | No references, nullable pointers only, destructor must handle nulls |
| Compile-time Enforcement | Ensures correctness and prevents unsafe zeroing                     |

---

## Conclusion

Zero initialization is a **powerful but dangerous feature** unless the language enforces safety rules.
This proposal introduces a **compiler-verified**, **explicit**, and **ergonomic** system for safely using zero-initialized types.

The combination of:

* `Zeroable`
* `zeroed(Type)`

provides a foundation for reliable, high-performance low-level initialization