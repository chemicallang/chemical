# `dyn` object model — fat pointers and boxed views

Status: design note (2026-11-01). This is the M1.19 starting point. It records
what the compiler implements today and the decision needed before `Box<dyn I>`
can become a safe owning API.

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
- The concrete value must outlive the box. The compiler does not yet model the
  boxed view as a borrow, so a safe `box(dyn<I>(local))` can escape a dead
  referent. Until that is fixed, constructing `Box<dyn I>` in safe code is
  rejected with `SAFE-DYN-001`; an explicit `unsafe { }` block documents the
  caller's obligation.
- `Box<dyn I>` does **not** run the concrete value's destructor. It frees only
  the fat-pointer record. A destructible referent must be owned elsewhere.

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
2. **A distinct owning erased type**, for example `DynBox<I>` /
   `OwnedDyn<I>`, with a vtable drop slot or a stored drop function. `Box<dyn I>`
   stays a boxed borrowed view. This preserves generic `Box<T>` and keeps
   `dyn` a pure fat pointer.
3. **A definition that `Box<dyn I>` is the owning form** and a new
   compiler-specialized layout. This is the Rust-compatible option, but it is
   the largest change and should wait for generic impl/layout support.

## Recommendation

Keep the invariant and separate the two concepts:

- `dyn I` — two-word borrowed fat pointer.
- `Box<dyn I>` — thin owning pointer to a heap fat-pointer **record** (boxed
  view). Safe construction only after boxed-view borrow tracking lands; until
  then `SAFE-DYN-001` requires `unsafe { }`.
- `DynBox<I>` (name to be decided) — owning erased value. It owns one concrete
  allocation and stores enough metadata to drop it and free it. This is the
  Rust-like `Box<dyn Trait>` role.

Recommended implementation order:

1. **Landed:** target-aware `dyn` size/alignment, `&dyn` dispatch through
   `Box::get`, `Box<dyn>` boxed-view test under `unsafe`, and `SAFE-DYN-001`.
2. Model `dyn` and `Box<dyn>` as borrow views in MIR so a boxed view cannot
   outlive its referent; remove the `unsafe` requirement once the escape check
   is proven.
3. Add the owning erased type: a vtable drop slot (or stored drop function),
   a checked `box_dyn<I>(value)` constructor, and drop/free glue. This must
   handle destructible and non-`Copy` values exactly once.
4. Then decide whether `Box<dyn I>` should remain a boxed view or be
   redefined/specialized as the owning type. Do not silently change its
   meaning later.
5. Continue object-safety work: `dyn I + Send/Sync` auto-trait bounds,
   upcasting (`dyn Derived` → `dyn Base`), and object-safe associated items.

## `&str` note

There is no `&str` type today. `std::string_view` is a byte view
(`ptr + len`) and `std::string` owns bytes; UTF-8 validation and `&str`
boundaries are M1.11, not part of the `dyn` fat-pointer work. Do not conflate
the two: `&str` is a string slice, while `dyn I` is a vtable-carrying object
view.

## Current test anchors

- `lang/tests/common/src/values/sizeof_alignof.ch` — two-pointer size/alignment
  on the active target, including comptime `sizeof`/`alignof`.
- `lang/tests/common/src/decls/interfaces/dynamic.ch` — by-value `dyn`
  parameters/returns/fields/arrays and `&dyn` dispatch.
- `lang/tests/src/stdlib/box.ch` — boxed `dyn` view under `unsafe`, including
  dispatch through `Box::get`.
- `lang/tests/negative/src/safety_diagnostics.ch` — safe `Box<dyn I>`
  construction is `SAFE-DYN-001`.
