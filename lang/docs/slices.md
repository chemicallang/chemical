# Slices and `&str` (first view slice)

Status: implemented as borrowed views over the existing pointer/length types.
`&[T]` is a parser alias for `std::span<T>`; `&str` is a parser alias for the
new `std::str`, a UTF-8-validated `std::string_view` wrapper. Both are
non-owning view values and are tracked by the existing MIR borrow-view analysis.

## `&[T]`

```chemical
func sum(values : &[int]) : int {
    var total = 0
    for(var i = 0u; i < values.size(); i++) {
        total += values[i]
    }
    return total
}

var arr : [4]int = [1, 2, 3, 4]
var values : &[int] = arr     // array view
var view : &[int] = &vec      // vector view

values.size()
values[2]
values.get(2)                 // *T in the current `span` API
```

- Representation: `std::span<T>` (`_data` pointer + `_size` length).
- Construction: the existing implicit constructors from an array or `&vector<T>`.
- Methods: `data`, `size`, `empty`, `get`, and the new `Index` impl for `[]`.
- A view does not own its elements. Returning one that borrows a local is
  rejected by `SAFE-LIFE-001`; outliving the referent is `SAFE-LIFE-002`.
- `&mut [T]` currently maps to the same view type and is read-only; mutable
  slice indexing is not implemented.

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

- No bare `[T]` unsized type and no `Box<[T]>` yet.
- No `&mut [T]` mutation/indexing.
- No slice pattern/destructuring syntax.
- `&str` has no separate lifetime parameter; the MIR view analysis keeps the
  source loan alive and rejects local escapes. A future `str<'a>`/`&'a str`
  spelling can make the lifetime explicit.
- `std::str` validation currently panics rather than returning a `Result`; use
  the checked view constructor only at a boundary where abort-on-invalid is
  acceptable. A `try_from_utf8` surface is future work.
- `&[T]` and `&str` depend on the `std` module at the type level.
