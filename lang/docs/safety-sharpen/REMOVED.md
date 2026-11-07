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

Tests: `./scripts/test.sh --tcc --removed`.
