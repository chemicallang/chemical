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
```

- `subview(start, end)` is bounds checked and panics on an invalid range.
- Shared subviews are `span<T>`; mutable subviews are `mut_span<T>`.
- `Box<[T]>` exposes `subview` and `subview_mut`.
- Range syntax (`view[a..b]`), `split_at`, `split`, `windows`, and `chunks` are
  not implemented yet.

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
- Methods: `data`, `size`, `empty`, `get`, `to_view`, `subview`, `starts_with`,
  `ends_with`.
- Returning a `&str` derived from a local `std::string` is rejected by
  `SAFE-LIFE-001`.

## Limits of this slice

- `[T]`/`&[T]` are view aliases, not a first-class unsized `[T]` type.
- `Box<[T]>` is a vector-backed owning wrapper, not a raw boxed allocation.
- No slice pattern/destructuring syntax, range sub-slicing (`view[a..b]`),
  `split_at`, `split`, or `windows`/`chunks` iterators yet.
- `&str` has no separate lifetime parameter; the MIR view analysis keeps the
  source loan alive and rejects local escapes. A future `str<'a>`/`&'a str`
  spelling can make the lifetime explicit.
- `std::str` validation currently panics rather than returning a `Result`; a
  checked `try_from_utf8` surface is future work.
- `[T]`, `&[T]`, `&mut [T]`, `Box<[T]>`, and `&str` depend on the `std` module
  at the type level.
