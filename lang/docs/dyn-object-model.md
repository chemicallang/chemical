# `dyn` object model — fat pointers and boxed views

Status: design note (2026-11-01). The first slice is implemented: `dyn` is a
target-aware fat pointer, `Box<dyn I>` is a borrow-checked boxed view, and
`std::DynBox<I>` provides owning type erasure. This note records the remaining
design choices (vtable drop slot vs. owner closure, auto traits, upcasting).

## The invariant

**`dyn I` is always a two-word fat pointer value.**

```
dyn I  ==  { data: *mut void, vtable: *const I_vt }
```

- The size is `2 * sizeof(*void)` and the alignment is pointer alignment, on
  every target. `sizeof(dyn I)` / `alignof(dyn I)` are tested in
  `lang/tests/common/src/values/sizeof_alignof.ch`.
- Indirections are written outside the `dyn`: `&dyn I` is a reference to the
  fat pointer, `*dyn I` is a pointer to it, and `[]dyn I` is an array of
  fat-pointer values.
- `dyn` is therefore a **copyable, non-owning view**. Copying it copies the
  data and vtable words; it never copies or destroys the referent.
- Interface vtables contain method slots only. They have no drop slot, so a
  bare `dyn I` cannot own or destroy its referent.
- `dyn<I>(value)` takes the address of `value` and builds the fat pointer. It
  does not allocate a copy of `value`.

This is already the implementation (`ast/types/DynamicType.h`,
`compiler/mir/MIRTypeBuilder.h` `fat_pointer_type`, and the MIR `DynamicValue`
case). It is also why `dyn` is not the right type for an owned erased value:
ownership needs a drop contract that the current vtable does not carry.

## What `Box<dyn I>` means today

Chemical's generic `Box<T>` is:

```
struct Box<T> { private var ptr: *mut T }
```

Therefore:

```
Box<dyn I>  ==  a thin owning pointer to a heap-allocated { data, vtable } record
```

`std::box<dyn I>(dyn<I>(value))` allocates and owns the **fat-pointer record**,
not the concrete `value`. The `data` word still points at `value`. This is a
boxed view, analogous to `Box<&T>`, not a Rust-style owning trait object.

Consequences:

- `Box<dyn I>::get()` returns `&dyn I`; dynamic dispatch through that reference
  works (`&dyn` is peeled to its fat pointer before the vtable call).
- The concrete value must outlive the box. MIR now models the boxed view as a
  borrow: returning a boxed view of a local is `SAFE-LIFE-001`, and a boxed view
  that outlives the referent's scope is `SAFE-LIFE-002`. Safe construction needs
  no `unsafe`.
- `Box<dyn I>` does **not** run the concrete value's destructor. It frees only
  the fat-pointer record. A destructible referent must be owned elsewhere.

## Owning `DynBox<I>`

`std::DynBox<I>` is the Rust-like owning erased value. It is a struct rather
than a special `Box<dyn I>` layout:

```
struct DynBox<I> {
    fat: dyn I,
    owner: std::function<() => void>
}
```

`std::dyn_box<T, I>(value)` allocates `T` once with `std::box`, builds a
`dyn I` view against that stable heap address, and moves the owning `Box<T>`
into an empty capturing closure. The closure's destructor drops the `Box<T>`,
so `DynBox` destroys the concrete value exactly once. `DynBox` itself is
move-only because the owner closure is destructor-bearing.

This costs a second allocation for the owner closure; a future vtable drop slot
can remove it. The important property is that `DynBox` is already a safe,
correct owning API and does not require changing `Box<T>`'s generic layout.
`DynBox` is deliberately excluded from the borrowed-view classification in MIR:
its `dyn` field points at heap storage that `owner` keeps alive.

## Rust comparison

Rust's `Box<dyn Trait>` is a *fat pointer*: data plus vtable, and the vtable has
drop glue. One allocation owns the concrete value. Chemical's generic `Box<T>`
always stores one pointer to `T`, so for `T = dyn I` it stores one pointer to a
two-word record. The two models differ in both indirection and ownership.

We cannot make `Box<dyn I>` match Rust without one of:

1. **A compiler-known specialization of `Box<dyn I>`** whose layout is a fat
   pointer instead of `{ *mut T }`. This breaks `Box<T>`'s uniform layout and
   needs generic specialization; it also changes `Box<T>`'s ABI for that one
   instantiation.
2. **A distinct owning erased type**, `DynBox<I>`, with a stored owner/drop
   contract. `Box<dyn I>` stays a boxed borrowed view. This preserves generic
   `Box<T>` and keeps `dyn` a pure fat pointer. Implemented using an owner
   closure; a vtable drop slot can optimize it later.
3. **A definition that `Box<dyn I>` is the owning form** and a new
   compiler-specialized layout. This is the Rust-compatible option, but it is
   the largest change and should wait for generic impl/layout support.

## Recommendation

Keep the invariant and separate the two concepts:

- `dyn I` — two-word borrowed fat pointer.
- `Box<dyn I>` — thin owning pointer to a heap fat-pointer **record** (boxed
  view), with the referent borrow checked by MIR.
- `DynBox<I>` — owning erased value; owns the concrete allocation and drops it
  exactly once through its owner closure.

Implementation status and next steps:

1. **Landed:** target-aware `dyn` size/alignment, `&dyn` dispatch through
   `Box::get`, MIR borrow tracking for `dyn`/`Box<dyn>`, safe `Box<dyn>` with
   `SAFE-LIFE-001`/`002` rejections, and `std::DynBox<I>` / `std::dyn_box`.
2. Optimize `DynBox` with a vtable drop slot (or equivalent stored drop
   function) to remove the owner-closure allocation; keep the owner-closure
   implementation as the correctness baseline.
3. `dyn I + Send/Sync` auto-trait bounds and `DynBox` Send/Sync conditionally.
4. Upcasting (`dyn Derived` → `dyn Base`) and object-safe associated items.
5. Continue M1.11 (`&str`/UTF-8) separately.

## `&str` note

There is no `&str` type today. `std::string_view` is a byte view
(`ptr + len`) and `std::string` owns bytes; UTF-8 validation and `&str`
boundaries are M1.11, not part of the `dyn` fat-pointer work. Do not conflate
the two: `&str` is a string slice, while `dyn I` is a vtable-carrying object
view.

## Current test anchors

- `lang/tests/common/src/values/sizeof_alignof.ch` — two-pointer size/alignment
  on the active target, including comptime `sizeof`/`alignof`.
- `lang/tests/common/src/decls/interfaces/dynamic.ch` — `dyn`
  parameters/returns/fields/arrays, `&dyn` dispatch, and referent-lifetime
  hygiene.
- `lang/tests/src/stdlib/box.ch` — safe boxed `dyn` view, `DynBox` owning
  drop-once behavior, and dispatch through `Box::get`/`DynBox::get`.
- `lang/tests/negative/src/safety_diagnostics.ch` — returning a boxed `dyn`
  view of a local is `SAFE-LIFE-001`; letting it outlive the referent is
  `SAFE-LIFE-002`.
