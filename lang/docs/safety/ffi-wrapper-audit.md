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
| `vector<T>` constructors | zero-sized elements still need a non-null, aligned sentinel | allocate one byte for `sizeof(T) == 0`; reserve grows capacity without `realloc` |
| `Rc::clone/downgrade`, `Weak::upgrade` | strong/weak counts cannot wrap to zero while handles remain | count-overflow panic before increment |
| `Arc::clone/downgrade`, `ArcWeak::upgrade` | same, under the count lock | count-overflow panic before increment |
| `u16_strlen` | dereferences a caller-provided UTF-16 pointer | now `unsafe func`; internal caller uses an explicit unsafe boundary |

Regression coverage: `vector_zero_sized_element_works`,
`vector_zero_sized_element_destructors_run`, and
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

## Tranche 13 — review-only ownership/guard items (2026-11-04)

| Surface | Review result |
|---|---|
| `string_view` raw constructors | `constructor`, `ref_make`, and `make_no_len` are already `unsafe`; safe `make_view`/`to_view` return `'self` views and the dangling-local/return cases are rejected by `SAFE-LIFE-001`/`SAFE-LIFE-002`. |
| `u16string` raw constructors | `constructor`, `constructor2`, and `make_no_len` are `unsafe`; `u16_strlen` is `unsafe`; safe append paths validate null and self-aliasing. |
| `Rc`/`Weak` and `Arc`/`ArcWeak` | counts cannot wrap; `upgrade` checks a live strong owner; `get_mut` requires zero weak handles; `try_unwrap` transfers the value once and leaves weak handles able to keep/free only the allocation. Runtime coverage already exercises weak + try-unwrap and upgrade/drop races. |
| `RefCell` guard escape | Guards are lifetime-bound to the cell, private-state, and `!Send`/`!Sync`. Leaking a guard leaves the cell permanently borrowed (subsequent `try_borrow` returns `None`, mutable borrows fail, the cell cannot be moved while borrowed); that is a safe leak/DoS, not UB. No language-level guard escape found. |

No code change was required for these items. `RefCell`/`Rc`/`Arc` remain covered
by the existing stdlib and auto-trait tests.

## Tranche 14 — Windows recursive tree removal and directory nulls (2026-11-04)

| Surface | Invariant | Enforcement |
|---|---|---|
| Windows `remove_dir_all_recursive_native` | `path + separator + entry + NUL` fits `WIN_MAX_PATH` | bounded path/name copies and checked suffix arithmetic before every write |
| Windows `remove_dir_all_recursive_native` | child buffer and find handle are released on every path | a `CustomDeletor<u16>` owns the child buffer; `FindClose` runs before each error return |
| Windows `utf8_to_utf16_inplace` | null input/output and zero capacity are rejected | `InvalidInput` before `MultiByteToWideChar` |
| POSIX/Windows native directory entry points | null paths never reach `open`/`mkdir`/`rmdir`/`RemoveDirectoryW`/`DeleteFileW` | `InvalidInput` |
| POSIX `remove_dir_all_recursive_native` | null path is rejected | `InvalidInput` |

The Windows recursive remover copied the original path and each `WIN32_FIND_DATAW`
name into a fixed `WIN_MAX_PATH` heap buffer without bounds checks, leaked the
child buffer on every call, and leaked the find handle if the child allocation
failed. It now fails with `PathTooLong` before overflow, owns the child buffer,
and closes the find handle on every exit.

Regression coverage: `test_fs_directory_rejects_null_inputs` (runs on POSIX;
the same public APIs are compiled for the i386 Windows target check).

## Tranche 15 — HTTP body ownership and raw wrappers (2026-11-04)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `http::Body.make_body` | the Body must not retain a pointer to caller stack storage | **`unsafe func`**; when `owns=false`, the already-read bytes are copied into a Body-owned `net::Buffer` |
| `http::Body.read` | null destination only valid for zero-length reads | `-1` for null with positive capacity; capacity above `INT_MAX` also rejected |
| `net::Buffer.as_ptr` | never index the backing vector at `read_pos == size` | returns null once all bytes are consumed |
| `http::ResponseWriter.send_file` | `open`/`CreateFileA` receive a NUL-terminated path | copies the view into a temporary `std::string`; rejects empty/null views |
| `http::strcasecmp` | raw C-string arguments cannot be length-validated | **`unsafe func`**; null/null and null/non-null return deterministic |
| `http::parse_request_from_bytes` | caller supplies the raw pointer/length | **`unsafe func`**; safe callers use the incremental reader |
| `http::read_response_incremental` | TLS context is a raw pointer and the caller controls the body limit | **`unsafe func`**; Body no longer borrows the parser scratch buffer |

The Body copy closes a safe-API lifetime hole: `read_request_incremental` and the
server pass a stack `net::Buffer`, while the resulting `Request` can outlive the
handler call. `make_body` now either takes ownership (`owns=true`) or copies the
already-read bytes into its own buffer. The client frees its parser scratch
buffer once the response Body has copied it. `Buffer.as_ptr` previously indexed
one past the vector after the header bytes were fully consumed (`size != 0`,
`read_pos == size`), which the new copy path exposed.

Regression coverage (LLVM TLS filter): `http_body_read_rejects_null_buffer`,
`http_body_copies_initial_buffer`, and `http_strcasecmp_handles_null`; the
existing invalid-URL/connection-refused checks also pass.

## Tranche 16 — TLS hostname/accept ownership (2026-11-04)

| Wrapper | Invariant | Enforcement |
|---|---|---|
| `ssl_set_hostname` | the SNI name must outlive the caller's string | copies up to 255 bytes into an SSLContext-owned buffer; `hostname_owned` controls `ssl_free` |
| `SSLContext` hostname ownership | replacing/freeing a context must not leak or double-free the copied name | init clears the flag; setter frees a previous owned copy; `ssl_free` frees the copy once |
| `tls_accept` | the accepted socket has one owner on success and failure | success transfers it to the SSLContext; every failure path closes or frees the context that owns it |
| HTTP `handle_conn` | the TLS context must not close the socket twice | `close_connection` suppresses `ssl_free`'s close and performs the single close itself |

`ssl_set_hostname` previously stored the caller's raw pointer. A caller could
pass a temporary `std::string` and start the handshake after it was destroyed;
SNI construction and certificate verification then read freed memory.
`tls_accept` also closed the accepted socket on handshake failure while the HTTP
server closed it again, allowing a double-close (and fd-reuse race).

Regression coverage: `tls_hostname_is_copied_and_owned`, plus the HTTP body and
client failure tests in the LLVM TLS filter.

## Tranche 17 — TLS caller-owned config lists and Windows IPC (2026-11-04)

| Surface | Invariant | Enforcement |
|---|---|---|
| `ssl_set_ca_chain` | CA chain is caller-owned and must outlive the handshake | **`unsafe func`**; null config no-op |
| `ssl_set_own_rsa_key` | private key is caller-owned and must outlive the handshake | **`unsafe func`**; null config no-op |
| `ssl_set_alpn_protocols` | protocol list and every string are caller-owned | **`unsafe func`**; null/zero list clears; null entries clear the list |
| `ssl_get_alpn_negotiated` | returned buffer is owned by the context | **`unsafe func`**; null context returns null |
| `ssl_set_config` | caller config may be a stack value or temporary | copies the config into SSLContext-owned storage; `ssl_free` releases one copy |
| Windows `send_message` | null handle/buffer and oversized lengths are rejected | returns without calling `WriteFile` |
| Windows test launch | pipe reads never index past the buffer | clamp `DWORD` counts before NUL append |
| Windows test launch | failed connect does not leak child handles | terminates the child and closes process/thread/pipe handles |

The TLS setters previously accepted raw caller storage through safe signatures.
They are now explicit unsafe boundaries; callers must keep the CA chain, private
key, and ALPN string array alive for the handshake. ALPN additionally rejects
null list entries instead of dereferencing them during ClientHello construction.
`ssl_set_config` now copies the config, so its pointer lifetime is owned by the
context (the raw CA/key/ALPN pointers inside it remain caller-owned).
The Windows test IPC path now fails closed on null/invalid handles and bounds
message sizes before `WriteFile`, and cleans up child handles when pipe connect
fails.

Regression coverage: the existing ALPN/CA/key setter tests and the i386 Windows
target safety check (`windows_i386_libs_safety`); LLVM TLS filtered tests `7/7`.

## Tranche 18 — macOS cstd and POSIX reactor (2026-11-04)

| Surface | Invariant | Enforcement |
|---|---|---|
| macOS `get_errno`/`set_errno` | `__error()` and the returned int are raw | unsafe-gated access |
| macOS `get_stdin`/`get_stdout`/`get_stderr` | `__stdinp`/`__stdoutp`/`__stderrp` are extern globals | unsafe-gated reads |
| macOS `get_environ` | `_NSGetEnviron()` and its result are raw | unsafe-gated call/dereference |
| `FdSet` helpers | fd must map inside the 1024-bit fd_set | bounds-check `0 <= fd < 1024` |
| select fallback | an out-of-range fd must not be silently probed | returns `POLLERR` for the entry instead of indexing |
| epoll/kqueue poll buffers | allocation failure/overflow cannot reach a null kernel buffer | checked size arithmetic and select fallback |
| epoll registration/probe | null scratch buffers are not written | null checks before event construction |

The macOS `cstd` sources were still safe wrappers over raw Darwin globals/functions
and failed the macOS target safety check. The POSIX reactor's fallback `fd_set`
helpers also indexed `fd / 64` without the 1024-bit bound, and the epoll/kqueue
poll/register paths used unchecked scratch allocations. Both are now fail-closed.

Target checks: `async`, `std`, `fs`, `net`, `tls`, `http`, `server`, and `test`
compile cleanly for `x86_64-apple-darwin` (and `arm64-apple-darwin` for the
async/std roots). Linux async suite `83/83`. FreeBSD's `cstd` source mapping was
added in Tranche 20.

## Tranche 19 — zero-sized vectors and UTF-8 bounds (2026-11-05)

| Surface | Invariant | Enforcement |
|---|---|---|
| `vector<T>` with `sizeof(T) == 0` | pointer arithmetic and element destructors still need a valid address | one-byte aligned sentinel per vector; zero-size capacity growth does not call `realloc` |
| zero-sized vector lifetime | destructors run once per logical element on clear/remove/delete | same destruction paths as non-zero-sized vectors; `remove`/`clear`/drop coverage |
| `fs::utf8_to_utf16` | continuation lookahead must not extend past the NUL terminator | measure `strlen` once and reject each sequence with `in_len - i` shorter than 2/3/4 before reading |
| `fs::utf16_to_utf8` | output capacity is checked before every 1–4 byte write | existing `pos + n >= out_len` checks remain |
| `u16string.append_u16_unit` / `append_codepoint` | the length arithmetic cannot wrap before `ensure_mut` | reject `offset` at/above the maximum unit count |
| `u16string.to_utf8` | the `n * 3 + 1` upper bound cannot wrap | reject `n > (SIZE_MAX - 2) / 3` |

Zero-sized vectors previously failed closed at construction. They now use a
one-byte sentinel (never a null pointer), skip `realloc` on capacity growth, and
still run per-element destructors. The UTF-8 path converter now bounds every
multi-byte lookahead by the measured input length instead of relying on NUL
short-circuit behavior. The UTF-16 append/to-UTF-8 entry points now reject
length arithmetic that would wrap before `ensure_mut`/allocation.

Regression coverage: TCC main `2489`, LLVM main `2487` (inline zero-sized
vector and destructor tests), safety suite `78/78` (zero-sized vector runtime),
TCC libs `736/736`, negative `818/818`. The UTF converter was also exercised by
a temporary public harness on both backends with ASCII, 2/3/4-byte, surrogate,
and truncated inputs.

## Tranche 20 — FreeBSD cstd, Windows IOCP, and target regressions (2026-11-05)

| Surface | Invariant | Enforcement |
|---|---|---|
| FreeBSD cstd | `__error`, standard streams, and `environ` are raw libc symbols | new `cstd/posix_bsd` wrappers; `link "c"` covers FreeBSD |
| Windows `CompletionPort` | a failed port/socket registration cannot post an orphaned operation | constructor/`register` fail closed; recv/send/accept complete with `-1` |
| Windows IOCP allocations | null operation/table/buffer allocations are not dereferenced | allocation checks; accept closes its socket on buffer failure |
| Windows IOCP lengths | raw buffer lengths fit `WSABUF.len` and null+positive-length is rejected | recv/send preflight checks |
| Windows IOCP entry points | raw buffer APIs are explicit unsafe boundaries | `async_iocp_recv`/`async_iocp_send` are `unsafe func` |
| Windows HTTP branches | Win32 extern calls/placement-new are audited; IOCP context buffers fail closed | `send_file`, static attributes, and the IOCP server branch use explicit unsafe blocks; null buffer/context allocations close the socket |
| Windows `std` | Win32 declarations have one owner | duplicate `GetCurrentThreadId` removed from the thread pool (cstd owns it) |
| Target regressions | a target check must compile, not merely avoid `SAFE-*` text | `--target ... --check` regression helper; modules include `http`/`server` |

The remaining parser audit is a sign-off item, not a code gap: `u16string`
append/to-UTF-8 length arithmetic is guarded and the `string_view` numeric
parsers already use bounded slices and explicit overflow checks.

Remaining target-source coverage: non-claimed OS mappings (`NetBSD`/`OpenBSD`)
and the Linux/Windows-only `window`/`webview` backends. The audit backlog below
is otherwise signed off for the claimed roots.

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
  `Cell`/`RefCell`, and thread-pool wrappers: parser/capacity handoff paths
  signed off for the claimed surface; keep new constructors on the same
  checked pattern.
- `fs::metadata` and remaining POSIX/Windows file APIs: reviewed; `fs::utf`
  lookahead and directory traversal/removal bounds are length/ownership-bounded.
- `net` Windows IOCP/async socket lifetime: allocation, length, registration,
  and HTTP Windows transport boundaries are now wrapped; address/error-code
  normalization remains a diagnostics item, not a safe-wrapper hole.
- `tls`/`http`/`server` transport wrappers: ownership, TLS caller lists, HTTP
  body/scratch buffers, and Windows IOCP/HTTP branches are audited.
- `cstd` remains the raw C surface; its functions stay `@extern`/unsafe and are
  not safe wrappers. New safe wrappers must not expose `*char`/`*mut` without a
  documented and enforced contract.
- Target-specific audit entries: Windows, macOS, and FreeBSD claimed roots are
  checked by strict target regressions; non-claimed OS mappings and
  Linux/Windows-only windowing/webview backends remain. The 94-module baseline
  remains host-oriented.
