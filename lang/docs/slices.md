# Slices and `&str` (borrowed and owning views)

Status: `[T]`/`&[T]` are borrowed slice views, `&mut [T]` is a mutable borrowed
view, `Box<[T]>` is the owning slice, and `&str` is a UTF-8-validated string
view. All four are parser aliases over std types; MIR borrow analysis tracks
the non-owning forms.

## Bare `[T]` and `&[T]`

```chemical
func sum(values : [int]) : int {
    var total = 0
    for(var i = 0u; i < values.size(); i++) {
        total += values[i]
    }
    return total
}

var arr : [4]int = [1, 2, 3, 4]
var bare : [int] = arr       // bare borrowed slice view
var shared : &[int] = arr    // explicit shared view
var vector_view : &[int] = &vec
```

- Representation: `std::span<T>` (`_data` pointer + `_size` length).
- `[T]` and `&[T]` both map to that view; `[T]` is not a Rust-style unsized
  type yet.
- Construction: the existing implicit constructors from an array or
  `&vector<T>`.
- Methods: `data`, `size`, `empty`, `get`, and `Index` for `view[i]`.
- Shared views are immutable: assigning through `&[T]`/`[T]` is rejected with
  `cannot assign through a shared slice view`.
- Returning a view of a local is rejected by `SAFE-LIFE-001`; outliving the
  referent is `SAFE-LIFE-002`.

## `&mut [T]`

```chemical
func fill(values : &mut [int]) {
    for(var i = 0u; i < values.size(); i++) {
        values[i] = 7
    }
}

var values = std::vector<int>()
values.push(1)
var view : &mut [int] = &mut values
view[0] = 4
fill(&mut values)
```

- Representation: `std::mut_span<T>` (mutable pointer + length).
- Construction is from `&mut vector<T>` or `&mut arr` (a mutable array
  reference). A `const` array is rejected at type check; passing an array
  value without `&mut` is rejected as well.
- Reads, writes, `set`, `get_ref`, `get_mut`, `as_span`, `subview`, `size`, and
  `Linear` iteration are supported.
- `mut_span<T>` is not `Copy`; the compiler's `IndexMut` interface now declares
  `index_mut` (the old `IndexMut : Index` overload produced duplicate C members
  and could not be implemented).
- A view that borrows a local vector cannot be returned (`SAFE-LIFE-001`).

## `Box<[T]>`

```chemical
var values = std::vector<int>()
values.push(5)
values.push(6)

var owned : Box<[int]> = values   // implicit move into the owning slice
owned.size()
owned[1]                          // indexed read
owned[1] = 9                      // indexed write
owned.set(0, 4)
owned.as_span()                   // borrowed span view
```

- `Box<[T]>` is parsed as `std::boxed_slice<T>`.
- It currently owns a contiguous `std::vector<T>` (the wrapper owns no raw
  allocation of its own); dropping it drops every element and frees the
  vector's storage exactly once.
- Construction is from a `vector<T>` via the implicit constructor. There is no
  array-to-boxed-slice constructor yet.
- `Index` and `IndexMut` are implemented; `get_ref`, `get_mut`, `data`,
  `size`, `empty`, `set`, `as_span`, `subview`, and `subview_mut` are available.
- It is not `Copy`; moving it transfers ownership.

## Sub-slicing

```chemical
var arr : [5]int = [1, 2, 3, 4, 5]
var view : [int] = arr
var middle = view.subview(1, 4)   // elements 1..4

var mutable_view : &mut [int] = &mut arr
var mutable_middle = mutable_view.subview(1, 4)
mutable_middle[0] = 9

var owned : Box<[int]> = values
var shared = owned.subview(1, 3)
var mutable = owned.subview_mut(1, 3)

var parts = view.split_at(2)
parts.first[0]
parts.second[0]

var middle = view[1..4]
var prefix = view[..2]
var suffix = view[2..]

var windows = view.windows(3)
windows.get(0)[0]
var chunks = view.chunks(2)
chunks.get(1)[0]

var parts = view.split(0)
parts.get(0)[0]

var predicate_parts = view.split_by(||(x : &int) : bool => *x == 0)
predicate_parts.get(0)[0]
```

- `subview(start, end)` is bounds checked and panics on an invalid range.
- Shared subviews are `span<T>`; mutable subviews are `mut_span<T>`.
- `Box<[T]>` exposes `subview` and `subview_mut`.
- `split_at(index)` returns `slice_pair<T>` (`mut_slice_pair<T>` for mutable
  views) with `first`/`second` fields.
- Range syntax is supported: `view[a..b]`, `view[..b]`, `view[a..]`, and
  `view[..]` lower to `subview(a, b)`.
- `windows(width)` and `chunks(width)` return `span_windows<T>` /
  `span_chunks<T>`; use `.size()` and `.get(i)` to read each subview.
- `split(delim)` returns `span_delim_split<T>` and requires `T: Copy +
  core::ops::PartialEq`; `split_by(predicate)` returns `span_split<T>` for a
  custom comparison. `&str`/`string_view` also have `.split(delim)`; the safe
  form returns owned `vector<string>` pieces. Use the `unsafe split_views`
  form only when the source outlives every returned view.

## `&str`

```chemical
func first(value : &str) : char {
    return value.get(0)
}

var text : &str = "hello"
var from_string : &str = &owned_string
var size = text.size()
var view = text.to_view()      // std::string_view
```

- Representation: `std::str` wraps a `std::string_view` (`ptr + length`).
- Construction validates UTF-8 and panics on invalid input. Invalid UTF-8 can
  be demonstrated with an unchecked `std::string_view`:
  `std::str.make_view(unsafe(std::string_view("\xFF", 1)))` aborts.
- `std::str` is a non-owning view; `&str` is a parser alias for it.
- It deliberately does not expose the unchecked `string_view` constructors.
- `std::try_from_utf8(view)` returns `Result<str, string_view>` and does not
  abort on invalid bytes.
- Methods: `data`, `size`, `empty`, `get`, `to_view`, `subview`, `starts_with`,
  `ends_with`.
- Returning a `&str` derived from a local `std::string` is rejected by
  `SAFE-LIFE-001`.

## Explicit lifetimes

Borrowed slices and `&str` accept a named lifetime:

```chemical
func <'a> identity(values : &'a [int]) : &'a [int] {
    return values
}

func <'a> text(value : &'a str) : &'a str {
    return value
}

func <'a, 'b> shorten(values : &'a [int]) : &'b [int] where 'a : 'b {
    return values
}
```

- The lifetime is carried on the `std::span` / `std::mut_span` / `std::str`
  generic type and is erased before MIR; MIR region signatures are built from
  the declaration before erasure.
- Returning `&'a [int]` as `&'b [int]` without a `where 'a : 'b` bound is
  rejected with `SAFE-LIFE-003`.
- Returning a shorter-lived slice or `&str` as `&'static` is rejected with
  `SAFE-LIFE-004`, including elided `&[T]`/`&str` sources whose written type
  has no lifetime argument.
- Elided forms (`&[T]`, `&mut [T]`, `&str`) keep their existing inference.

## Limits of this slice

- `[T]`/`&[T]` are view aliases, not a first-class unsized `[T]` type.
- `Box<[T]>` is a vector-backed owning wrapper, not a raw boxed allocation.
- Fixed-size arrays support flat patterns (`var [a, b] = arr`,
  `[first, .., last]`, `_`). `[T]`/`&[T]` support exact refutable condition
  patterns (`if(var [a, b] = view)`); slice rest patterns, `&mut [T]`/`Box<[T]>`
  patterns, and statement forms with an `else` path are not implemented yet.
- `windows`/`chunks` cursors are not `for`-iterable yet (use `.get(i)`).
- Explicit `&'a [T]`, `&'a mut [T]`, and `&'a str` forms are supported; the
  elided forms rely on MIR view analysis to keep the source loan alive and
  reject local escapes.
- The checked `std::try_from_utf8` surface exists; the `str` convenience
  constructors still panic on invalid bytes.
- `[T]`, `&[T]`, `&mut [T]`, `Box<[T]>`, and `&str` depend on the `std` module
  at the type level.
