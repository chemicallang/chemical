# Removed features

Phase 1 removes low-value language surface. The compiler emits these stable
codes with a migration hint; there are no silent aliases.

| Code | Removed syntax | Migration |
|---|---|---|
| `REMOVED-001` | associated type declarations/projections/bounds (`type Item`, `Self::Item`, `T::Item`, `Trait<Item = U>`) | add a generic parameter to the interface/function |
| `REMOVED-002` | external/generic inherent `impl Type { ... }` blocks | declare methods in the type body or use `impl Trait for Type` |
| `REMOVED-003` | nested tuple/array destructuring patterns | bind the outer value and destructure it in separate steps or use indexing |
| `REMOVED-004` | array/slice patterns in conditions and slice-view patterns | use a normal condition plus `view.size()`/`get(i)`/`subview` |
| `REMOVED-005` | `Box<[T]>` / `std::boxed_slice<T>` | use `vector<T>` for ownership or `[T]`/`&[T]` for a view |
| `REMOVED-006` | range-slicing syntax (`view[a..b]`, `view[..b]`, `view[a..]`, `view[..]`) | use `view.subview(a, b)` / `split_at` |

Kept: `dyn`/`DynBox`/dynamic calls, tuple structs, unit tuple, core tuples,
flat tuple destructuring, flat fixed-array exact/rest destructuring, fixed-array
`[T]`/`&[T]`/`&mut [T]`/`&str` views, generic impls, and trait impls.

## Library removals (no code, compiler error or missing symbol)

| Removed | Migration |
|---|---|
| `core::stream::Stream::writeStrNoLen(&self, *char)` | a formatter always knows the extent it renders; print a `string_view`/`str`/`string` instead, or read the length yourself and use `writeStr(ptr, len, spec)` |
| `core::stream::Stream::write*(value)` without a spec | the eight `Stream` methods take `spec : u64` (`0` = plain); the intrinsic passes it |
| `func stream(&self, s : &mut std::StringStream)` (two parameters) | `func stream(&self, s : &mut std::StringStream, spec : u64)` if you want format specs applied to your type; a two-parameter hook still compiles but rejects `${x:spec}` |
| `std::print`/`println` returning `any` | they return nothing (a console write is a statement) |
| `env.error(&str)` / `info` / `warning` / `warn` / `success` / `panic` | the message is an `%expressive_string`: pass a template or a literal directly (`env.error(`x is ${v}`)`, `env.error("boom")`); interpolate a runtime `str`/`string`/`string_view` (`${value}`) |
| `CommandLineStream::double_to_chars` / `float_to_chars` (internal) | `std::string::append_double` / `append_float` are the one double/float formatter |

`println(`...${rawPointer}`)` is now a compile error: a bare `*char` has no
length, so rendering it would scan memory the compiler cannot bound. See the
printing/formatting entry in [`BREAKING.md`](BREAKING.md).

Tests: `./scripts/test.sh --tcc --removed`.
