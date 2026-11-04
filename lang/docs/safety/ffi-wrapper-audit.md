# Semantic FFI Wrapper Audit

This document tracks the audit required by
`lang/docs/safety-implementation-plan.md` §4.5 item 2 and §10.4. The C ABI
classifier work answers "can the compiler pass this value correctly?"; this
audit answers "does the safe wrapper preserve its own pointer, length,
ownership, and lifetime preconditions?"

## Audit rule

A safe function may call an `@extern`/raw-pointer API only when every
caller-controlled invariant is either:

1. enforced by the wrapper before the boundary, or
2. reflected in the function's type/contract (`unsafe func`, a safe owning
   wrapper such as `string`, or an explicit `Layout`/`Result`).

Safe wrappers must not rely on an unstated precondition that a safe caller can
violate. Raw pointers obtained from safe APIs (for example `string.data()`) can
still dangle if used after the owner is dropped; wrappers that dereference them
must either keep the dereference behind `unsafe` or use a lifetime-carrying
view.

## Tranche 1 — allocator, filesystem paths/files, socket buffers (2026-11-03)

### `std::alloc`

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `GlobalAllocator.alloc` | valid `Layout`, non-null result for any size | `Layout.validate`; zero-size requests allocate one byte so the pointer is freeable |
| `GlobalAllocator.dealloc` | pointer came from this allocator with the same layout | **`unsafe func`** — `free` cannot validate provenance |
| `GlobalAllocator.grow/shrink` | old pointer is live; new size is non-zero | **`unsafe func`**; zero-sized grow/shrink returns `AllocError.InvalidLayout` instead of `realloc(ptr, 0)` (which may free while returning an error) |
| `GlobalAllocator.alloc_zeroed` | no null memset for zero size | zero-size memset skipped |
| debug tracker | allocation failure does not dereference null | tracker/record null checks |
| `alloc_bytes` | zero-size result is freeable | zero-size request allocates one byte |
| `dealloc_bytes` | pointer has matching allocation provenance | **`public unsafe func`** |

### `fs::path` and `fs::file_io`

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `basename`/`dirname`/`extension`/`join_path`/`normalize_path` | non-null path and output; output capacity includes the NUL | null checks and `>= out_len` checks |
| `join_path` | `a_len + separator + b_len + NUL` does not overflow | checked addition; `PathTooLong` |
| `atomic_write` | null path/data; temp path fits the stack buffer | null checks; capacity check before prefix/suffix writes |
| `read_entire_file`/`read_to_buffer`/`write_text_file`/`move_path` | non-null path; non-null buffer when length is non-zero | `InvalidInput` |
| `file_open`/`file_close`/`file_read`/`file_write`/`file_flush` | non-null handle; non-null buffer when length is non-zero | `InvalidInput` on POSIX and Windows |
| POSIX `create_temp_file_in_native` | directory/prefix/output fit `PATH_MAX_BUF` and caller output length | bounded scans and capacity checks before template writes |

### `net`

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `recv_all` | non-null buffer for positive length; length fits signed `int` | `-1` on violation |
| `send_all` | non-negative length; non-null buffer for positive length | `-1` on violation |
| `dial` | non-null address; port is a valid TCP/UDP port | rejects null and `> 65535`; bounded port string |
| `recv_async`/`send_async` | same buffer/length rules as the synchronous path | `-1` on violation |

## Tranche 2 — ownership wrappers and strings (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `Box::box` | zero-sized `T` still yields a non-null, freeable allocation | zero-sized values allocate one byte |
| `DynBox` | `fat`/`data`/`drop` stay synchronized and only `dyn_box` can construct them | fields are `internal` (not constructible outside `std`) |
| `string.append_with_len` | a null source is only valid for a zero-length append | zero length returns; null with a positive length panics |
| `string.append_char_ptr` | source is non-null before `strlen` | null panics |
| `string.append_str` | source string pointer is non-null | null panics |

Regression coverage:

- `box_zero_sized_value_allocates_freeable_storage`
- `neg_dyn_box_fields_private` (`cannot access internal member`)
- `string_append_null_panics`

## Tranche 3 — collections, reference counts, UTF-16 helper (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `vector<T>` constructors | zero-sized elements would make pointer arithmetic/indexing invalid | fail closed with a runtime panic instead of creating a malformed vector |
| `Rc::clone/downgrade`, `Weak::upgrade` | strong/weak counts cannot wrap to zero while handles remain | count-overflow panic before increment |
| `Arc::clone/downgrade`, `ArcWeak::upgrade` | same, under the count lock | count-overflow panic before increment |
| `u16_strlen` | dereferences a caller-provided UTF-16 pointer | now `unsafe func`; internal caller uses an explicit unsafe boundary |

Regression coverage: `vector_zero_sized_element_panics` and
`neg_u16_strlen_requires_unsafe`.

## Tranche 4 — thread-pool Future ownership (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `concurrent.Future<T>` | the promise pointer is live and owned by the future | `p` is private; constructor is `unsafe`; `get` panics on an empty future instead of dereferencing null |
| `concurrent.Promise<T>` | only the pool creates and synchronizes promises | fields and constructor are `internal` to `std` |

Regression coverage: `neg_threadpool_future_constructor_requires_unsafe`
(`SAFE-UNSAFE-002`).

## Tranche 5 — vector replacement and filesystem buffers (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `vector::set` | replacing an element drops the old value exactly once | `destruct get_ptr(index)` before storing the new value |
| `fs::temp_dir` | output non-null and large enough for `/tmp` plus NUL | null and capacity rejection |
| `fs::copy_directory` | constructed child paths fit `PATH_MAX_BUF` | bounded copies and capacity checks before appending names |
| Windows `fs::read_dir` | `pattern` fits `WIN_MAX_PATH` | bounded path copy and capacity checks |

Regression coverage: the vector destructor-counter replacement test and
`test_fs_temp_dir_rejects_small_or_null_output`.

## Tranche 6 — string_view numeric parsers (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `string_view::to_i8/i16/i32/i64` | parsed value fits the target width | magnitude accumulation with per-type limits; signed min handled without overflowing negation |
| `string_view::to_u8/u16/u32/u64` | parsed value fits the unsigned width | magnitude accumulation with `u64` max and per-type range checks |
| `string_view::to_int/to_uint/to_i32/to_u32` | narrow conversions do not truncate | underlying parser returns `Err("out of range")` before the cast |

Regression coverage: `lang/tests/src/stdlib/datatype/conversions.ch` checks
both limits and one-past-limit inputs for i8/i16/i32/i64/u8/u16/u32/u64,
including `i64::MIN` and `u64::MAX`.

## Tranche 7 — thread-pool promise result slot (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `concurrent.Promise<T>` | the result slot never contains an uninitialized `T` | `val` is an `Option<T>` initialized to `None`; the worker stores `Some(result)` |
| `concurrent.Future<T>::get` | moving the result out leaves the promise slot empty | `Option::take` moves the value out before `Promise` is deleted |
| `concurrent.Future<T>::delete` | dropping a ready future destroys a completed result exactly once | the promise destructor drops the `Option` payload; an unfinished future marks `future_dropped` and lets the worker perform the single free |

Previously `Promise<T>` copied an uninitialized `var xx : T` into `val`, so the
worker's `val = result` assignment and the promise destructor ran `T`'s
destructor on garbage bytes (a double-free for `string`-like results).

Regression coverage: `test_pool_future_drops_result_once` submits a
destructor-bearing result and asserts it is dropped exactly once.

## Tranche 8 — safe containerized string splitting (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `string_view::split` | returned pieces cannot dangle past the source | safe form returns owned `vector<string>` copies; the container no longer hides the source loan |
| `string::split` | same, for the owning string wrapper | delegates to the owned `string_view::split` |
| `string_view::split_views` / `string::split_views` | zero-copy pieces require the source to outlive every view | **`unsafe func`**; safe callers must use the owned form |

Previously `split` returned `vector<string_view>`; the nested view type has no
lifetime the borrow checker can tie to `&self`, so returning the vector from a
local string compiled and produced dangling views. The safe API now copies
pieces; the zero-copy path is an explicit unsafe boundary.

Regression coverage: `string split returns owned pieces`,
`string_view split returns owned pieces`, `split pieces outlive the source`,
and `neg_split_views_requires_unsafe`.

## Tranche 9 — u16string append aliasing and null sources (2026-11-03)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `u16string.append_with_len` | a null source is only valid for a zero-length append | zero length returns; null with a positive length panics |
| `u16string.append_with_len` | a source aliasing the string's own storage survives the storage move | source byte offset is recomputed after `ensure_mut`/`move_data_to_heap` |
| `u16string.append_utf8_view` | source pointer is non-null before decoding | null with a positive length panics |

Regression coverage: `u16 string self append survives storage move` forces an
SSO -> heap move while appending the string to itself; the safety suite adds
`u16_string_append_null_panics` and `u16_string_append_utf8_null_panics`.

## Tranche 10 — map `find` ownership (2026-11-04)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `unordered_map::find` | the output copy must not duplicate ownership of a stored value | **`where Value : Copy`**; owning/destructor values are rejected at the call site |
| `ordered_map::find` | same | **`where Value : Copy`** |

`find` writes `*value = *ptr` through a raw pointer. Before this tranche the
generic method accepted any `Value`, so a map value with a destructor (for
example a heap-backed `string` or a custom resource) was bitwise duplicated:
the map and the caller slot both owned the same pointer and both destructors
freed it. A small destructor-bearing test reproduced a glibc double-free.
`Copy` values keep the existing API; callers that need a borrowed view use
`get_ptr` (raw dereference remains `unsafe`).

Regression coverage: `neg_unordered_map_find_non_copy_value` and
`neg_ordered_map_find_non_copy_value`.

## Tranche 11 — worker-owned thread-pool teardown (2026-11-04)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `ThreadPool::delete` | an owner joins workers before freeing; a worker never joins itself | `PoolData` records each worker's OS thread id; an owner performs the join/free path, a worker defers shutdown |
| `PoolData` teardown | exactly one party frees the shared block | the last worker drains the queue, joins the other handles, and frees; owner teardown sets `owner_frees` so workers do not free |
| `ThreadPool::delete` receiver | destruction needs unique ownership | changed from `&self` to `&mut self` |

A safe task can capture the last `Arc<ThreadPool>` (the capture witness accepts
it because `ThreadPool` is audited `Send + Sync`). When the task finished, the
Arc destructor ran on a pool worker, the old destructor called `pthread_join` on
itself, then freed `PoolData` while the worker loop still used it. Reproduced as
a glibc heap corruption/abort.

Regression coverage: `test_pool_last_owner_dropped_on_worker`.

## Tranche 12 — POSIX filesystem bounds/null and UTF-16 (2026-11-04)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `fs::read_dir` | `path + separator + entry + NUL` fits `PATH_MAX_BUF` | null rejection and checked arithmetic before every stack-buffer write |
| `fs::metadata` | a null path is not passed to `lstat` | `InvalidInput` |
| `utf8_to_utf16` | null input/output is rejected | `InvalidInput` |
| `utf16_to_utf8` | null input/output and unpaired surrogates are rejected | `InvalidInput`; high surrogates require a low-surrogate follower |

`read_dir` previously copied `path` and the `dirent` name into a fixed
`PATH_MAX_BUF` stack buffer without checking either length; a long-but-valid
directory path plus a long entry name could overwrite the stack. The path is
now bounded before the slash and name/NUL are appended.

Regression coverage: `test_fs_directory_rejects_null_inputs`.

Still remaining in this group: actual zero-sized vector support,
`string_view`/`u16string` raw constructors, remaining `Rc`/`Arc` weak-handle
invariants, `RefCell` guard escape, and Windows UTF-16/tree removal.

## Coverage

- `lang/tests/src/stdlib/alloc/alloc_tests.ch`: zero-size freeable allocation;
  grow-to-zero rejection with old-pointer validity.
- `lang/tests/negative/src/safety_diagnostics.ch`: `dealloc_bytes` requires
  `unsafe` (`SAFE-UNSAFE-002`).
- `lang/tests/libs/fs/path_test.ch`: near-`PATH_MAX` atomic-write temp path;
  null path/component/buffer rejection.
- `lang/tests/async/net_test.ch`: invalid port and null-buffer rejection.

## Remaining audit backlog

- `std::vector`, `std::string`, `std::string_view`, `Box`, `Rc`/`Arc`,
  `Cell`/`RefCell`, and thread-pool wrappers: audit length/capacity, aliasing,
  zero-sized types, and ownership handoff.
- `fs::directory`, `fs::metadata`, `fs::utf`, Windows UTF-16 conversions, and
  remaining POSIX/Windows file APIs.
- `net` Windows IOCP, async socket lifetime/close rules, and address/socket
  error-code normalization.
- `tls`/`http`/`server` transport wrappers: buffer ownership and handshake
  lifetime rules.
- `cstd` remains the raw C surface; its functions stay `@extern`/unsafe and are
  not safe wrappers. New safe wrappers must not expose `*char`/`*mut` without a
  documented and enforced contract.
- Target-specific audit entries for non-Windows OS-gated sources; the current
  94-module list is host-oriented.
