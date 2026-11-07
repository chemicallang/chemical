# Safe-API / language breaking changes

This release is allowed to break. The migration mapping is in
[`REMOVED.md`](REMOVED.md) and will be expanded per Phase 2 library job.

## Phase 1 (language surface)

- Associated types and associated-type equality constraints are removed.
- External/generic inherent `impl Type { ... }` blocks are removed.
- Nested tuple/array destructuring and array/slice patterns in conditions are removed.
- Slice-view patterns are removed; use `size()`/`get(i)`.
- `Box<[T]>` and `std::boxed_slice<T>` are removed; use `vector<T>` or `[T]` views.
- Range-slicing syntax is removed; use `subview`/`split_at`.
- `dyn`/`DynBox`, tuple structs, unit tuple, core tuples, flat destructuring,
  slices/`str`, and generic impls remain.

## Release artifact

The release `TCCCompiler` is now built with size-oriented flags (`-O2`,
function/data sections, gc-sections, stripped) and is under the 5 MB hard
ceiling. See `metrics.json`.
