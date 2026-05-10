# Iteration Interfaces for the Language

This document defines the container iteration model for the language, with a focus on **performance**, **safety**, and **compiler-friendly lowering**.

The goal is to support these loop forms:

* `for x in container`
* `for x in container reversed`
* `for x, i in container`
* `for x, i in container reversed`

while preserving the fastest possible path for contiguous containers, a second fast path for segmented containers, and a general fallback for all other iterable structures.

---

## Design Goals

1. **Fastest path for contiguous memory**

    * `vector`, `span`, arrays, and similar containers should iterate with pointer arithmetic only.

2. **Second fast path for segmented storage**

    * Containers like deque-like structures, block-based buffers, and other segmented layouts should avoid fully generic iterators when possible.

3. **General fallback for all other containers**

    * Hash maps, trees, linked structures, graphs, and custom containers should still support iteration safely.

4. **Reverse iteration should be explicit**

    * Not every container can reverse efficiently.
    * Reverse support should be a capability, not assumed.

5. **Indexing should be cheap when real, and synthetic when not**

    * For contiguous and segmented containers, index is meaningful.
    * For generic iterables, `i` is only the iteration count.

---

# 1. Capability Ladder

The compiler should lower iteration in this order:

1. **Linear**
2. **Chunked / Segmented**
3. **Iterable**

Reverse variants are only used when the type implements the reverse-capable interface.

This gives the compiler the best possible implementation without forcing every container into one shape.

---

# 2. Contiguous Storage: `Linear<T>`

This interface is for containers whose elements are stored in one contiguous range.

```c
public namespace core::iterable {

public interface Linear<T> {

    func data(&self) : *const T

    func size(&self) : u64

}

}
```

### Meaning

* `data()` points to the first element.
* `size()` returns the number of elements.
* Elements occupy a contiguous block: `data() ... data() + size()`.

### Supported loop forms

* `for x in container`
* `for x in container reversed`
* `for x, i in container`
* `for x, i in container reversed`

### Compiler lowering: `for x in container`

```c
T* ptr = container.data();
T* end = ptr + container.size();

while (ptr != end) {
    T& x = *ptr;
    ptr += 1;
}
```

### Compiler lowering: `for x in container reversed`

```c
T* begin = container.data();
T* ptr = begin + container.size();

while (ptr != begin) {
    ptr -= 1;
    T& x = *ptr;
}
```

### Compiler lowering: `for x, i in container`

```c
T* ptr = container.data();
T* end = ptr + container.size();
u64 i = 0;

while (ptr != end) {
    T& x = *ptr;
    ptr += 1;
    i += 1;
}
```

### Compiler lowering: `for x, i in container reversed`

```c
T* begin = container.data();
u64 i = container.size();
T* ptr = begin + i;

while (ptr != begin) {
    ptr -= 1;
    i -= 1;
    T& x = *ptr;
}
```

### Notes

* This is the fastest general-purpose iteration model.
* The compiler can emit very tight machine code.
* This path should always be preferred over any iterator object.

---

# 3. Segmented Storage: `Chunked<T>`

This is the second path.

It is for containers that are not fully contiguous, but can expose contiguous runs of elements.

Examples:

* deque-like containers
* block-based dynamic buffers
* rope-like storage
* page-based collections

The core idea is that the container provides an iterator over **chunks**, and each chunk is itself linear.

```c
public namespace core::iterable {

public struct Chunk<T> {
    func data(&self) : *const T
    func size(&self) : u64
}

public interface Chunked<T> {

    type ChunkCursor

    func begin_chunks(&self) : ChunkCursor
    func valid_chunk(&self, c: ChunkCursor) : bool
    func current_chunk(&self, c: ChunkCursor) : Chunk<T>
    func next_chunk(&self, c: ChunkCursor) : ChunkCursor

}

}
```

### Meaning

* The container is divided into contiguous runs.
* Each chunk is a small `Linear<T>`-like range.
* The compiler can emit a nested loop: outer loop over chunks, inner loop over raw pointers.

### Supported loop forms

* `for x in container`
* `for x in container reversed`
* `for x, i in container`
* `for x, i in container reversed`

### Compiler lowering: `for x in container`

```c
ChunkCursor cc = container.begin_chunks();

while (container.valid_chunk(cc)) {
    Chunk<T> chunk = container.current_chunk(cc);

    T* ptr = chunk.data();
    T* end = ptr + chunk.size();

    while (ptr != end) {
        T& x = *ptr;
        ptr += 1;
    }

    cc = container.next_chunk(cc);
}
```

### Compiler lowering: `for x, i in container`

For segmented storage, `i` is a true logical index if the container defines a stable element order.

```c
ChunkCursor cc = container.begin_chunks();
u64 i = 0;

while (container.valid_chunk(cc)) {
    Chunk<T> chunk = container.current_chunk(cc);

    T* ptr = chunk.data();
    T* end = ptr + chunk.size();

    while (ptr != end) {
        T& x = *ptr;
        ptr += 1;
        i += 1;
    }

    cc = container.next_chunk(cc);
}
```

### Compiler lowering: `for x in container reversed`

Reverse order means:

1. Visit chunks in reverse chunk order.
2. Visit elements inside each chunk in reverse.

This requires a reverse-capable chunk cursor.

```c
ChunkCursor cc = container.rbegin_chunks();

while (container.valid_chunk(cc)) {
    Chunk<T> chunk = container.current_chunk(cc);

    T* begin = chunk.data();
    T* ptr = begin + chunk.size();

    while (ptr != begin) {
        ptr -= 1;
        T& x = *ptr;
    }

    cc = container.previous_chunk(cc);
}
```

### Compiler lowering: `for x, i in container reversed`

```c
ChunkCursor cc = container.rbegin_chunks();
u64 i = container.total_size();

while (container.valid_chunk(cc)) {
    Chunk<T> chunk = container.current_chunk(cc);

    T* begin = chunk.data();
    T* ptr = begin + chunk.size();

    while (ptr != begin) {
        ptr -= 1;
        i -= 1;
        T& x = *ptr;
    }

    cc = container.previous_chunk(cc);
}
```

### Notes

* This path is excellent when memory is mostly contiguous but split into manageable runs.
* The inner loop remains pointer-based and fast.
* Reverse iteration is supported when the container can enumerate chunks backwards.

---

# 4. General Fallback: `Iterable<T>`

This is for containers that cannot expose a useful contiguous or chunked representation.

Examples:

* unordered maps
* trees
* linked lists
* graph traversals
* custom node-based structures

```c
public namespace core::iterable {

public interface Iterable<T> {

    type Cursor

    func begin(&self) : Cursor
    func valid(&self, c: Cursor) : bool
    func current(&self, c: Cursor) : &T
    func next(&self, c: Cursor) : Cursor

}

}
```

### Meaning

* The container exposes an opaque cursor type.
* The cursor is concrete at compile time for the container.
* The compiler can inline the methods and avoid dynamic dispatch if possible.

### Supported loop forms

* `for x in container`
* `for x, i in container`

Reverse iteration is only available if the container implements the reverse-capable interface below.

### Compiler lowering: `for x in container`

```c
Cursor c = container.begin();

while (container.valid(c)) {
    T& x = *container.current(c);
    c = container.next(c);
}
```

### Compiler lowering: `for x, i in container`

```c
Cursor c = container.begin();
u64 i = 0;

while (container.valid(c)) {
    T& x = *container.current(c);
    c = container.next(c);
    i += 1;
}
```

### Notes

* The index `i` here is not a memory offset.
* It is simply the iteration count.
* This is still useful for enumeration, progress, and stable loop ordering.

---

# 5. Reverse-Capable General Iteration: `ReversibleIterable<T>`

Some general containers can iterate in reverse.

Examples:

* doubly linked lists
* some tree traversals
* ordered structures with bidirectional cursors

```c
public namespace core::iterable {

public interface ReversibleIterable<T> : Iterable<T> {

    func rbegin(&self) : Cursor
    func previous(&self, c: Cursor) : Cursor

}

}
```

### Supported loop forms

* `for x in container reversed`
* `for x, i in container reversed`

### Compiler lowering: `for x in container reversed`

```c
Cursor c = container.rbegin();

while (container.valid(c)) {
    T& x = *container.current(c);
    c = container.previous(c);
}
```

### Compiler lowering: `for x, i in container reversed`

```c
Cursor c = container.rbegin();
u64 i = container.count();

while (container.valid(c)) {
    T& x = *container.current(c);
    c = container.previous(c);
    i -= 1;
}
```

If the container cannot naturally know its count cheaply, then reverse-indexed iteration should either be unsupported or require a separate length method.

---

# 6. Associative Containers

A map-like container should not only be iterable; it should also expose associative lookup.

```c
public namespace core::iterable {

public interface Associative<K, V> {

    func get(&self, key: &K) : Option<&V>
    func contains(&self, key: &K) : bool

}

public interface MutableAssociative<K, V> {

    func get_mut(&mut self, key: &K) : Option<&mut V>
    func insert(&mut self, key: K, value: V)
    func remove(&mut self, key: &K) : bool

}

}
```

For example, an unordered map would typically implement:

* `Iterable<Entry<K, V>>`
* `Associative<K, V>`

---

# 7. Recommended Interface Set

## Fast contiguous path

```c
Linear<T>
```

Optional extensions:

```c
ReversibleLinear<T>
RandomAccess<T>
```

## Fast segmented path

```c
Chunked<T>
```

Optional reverse support through reverse chunk cursors.

## General path

```c
Iterable<T>
ReversibleIterable<T>
```

## Associative path

```c
Associative<K, V>
MutableAssociative<K, V>
```

---

# 8. What the Compiler Should Prefer

The compiler should lower loops by capability, in this order:

1. **Linear** if available
2. **Chunked** if available and linear is not available
3. **Iterable** otherwise

Reverse forms should be accepted only if the corresponding reverse-capable interface exists.

This is important because it allows the compiler to preserve the most efficient machine code possible for each storage layout.

---

# 9. Loop Semantics Summary

## `for x in container`

* Uses the best available path.
* Prefer `Linear`, then `Chunked`, then `Iterable`.

## `for x in container reversed`

* Uses reverse linear iteration if the container is `Linear`.
* Uses reverse chunk order plus reverse element order if the container is `Chunked`.
* Uses `ReversibleIterable` if the container is a general reverse-capable iterable.

## `for x, i in container`

* `i` is a real element index for linear and segmented containers.
* `i` is an iteration count for generic iterables.

## `for x, i in container reversed`

* Supported for `Linear` and `Chunked`.
* Supported for `ReversibleIterable` only if the container provides a reverse cursor and a meaningful count.

---

# 10. Suggested Container Mapping

| Container              | Interfaces                                                                      |
| ---------------------- | ------------------------------------------------------------------------------- |
| `vector`               | `Linear<T>`, `ReversibleLinear<T>`, `RandomAccess<T>`                           |
| `span`                 | `Linear<T>`, `ReversibleLinear<T>`, `RandomAccess<T>`                           |
| fixed array            | `Linear<T>`, `ReversibleLinear<T>`, `RandomAccess<T>`                           |
| `deque`                | `Chunked<T>`                                                                    |
| segmented buffer       | `Chunked<T>`                                                                    |
| doubly linked list     | `Iterable<T>`, `ReversibleIterable<T>`                                          |
| singly linked list     | `Iterable<T>`                                                                   |
| `unordered_map`        | `Iterable<Entry<K, V>>`, `Associative<K, V>`                                    |
| ordered map / tree map | `Iterable<Entry<K, V>>`, `ReversibleIterable<Entry<K, V>>`, `Associative<K, V>` |

---

# 11. Final Recommendation

Use **three iteration families**:

* `Linear` for contiguous memory
* `Chunked` for segmented memory
* `Iterable` for everything else

Then add reverse-capable variants only where the storage layout supports them naturally.

This gives the language:

* very fast loops for the common case,
* a strong middle path for segmented containers,
* and a safe fallback for all other structures.