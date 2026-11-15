# Release readiness: API freeze and open decisions

This document is the contract for the release that ports existing Chemical
code onto the safety-checked libraries. It records

1. what is **frozen** (users may write against it and expect no future break),
2. what **changed in this release** (the one-time breaking window), and
3. what is **known-missing**, with the reason it is additive (so it can land in
   a later release without touching user code).

The goal: later releases improve implementation, performance, and add backends
without forcing large user rewrites.

- Safety status: [`safety-sharpen/STATUS.md`](safety-sharpen/STATUS.md)
- Migration mapping: [`safety-sharpen/BREAKING.md`](safety-sharpen/BREAKING.md)
- API audit gates: [`safety-sharpen/API_AUDIT.md`](safety-sharpen/API_AUDIT.md)

---

## 1. Frozen API decisions

These are permanent. Changing any of them later is a breaking change, so they
were settled before the freeze.

### 1.1 Raw pointers require `unsafe func`

A public function whose **parameter list** mentions a raw pointer is
`unsafe func`. A raw-pointer **return** stays safe.

- Exempt: `%runtime<T>` / `%maybe_runtime<T>` parameters (materialized by an
  `@expand` template, never caller-supplied) and `__chc_*` compiler ABI hooks.
- Enforced by `scripts/safety-raw-surface-audit.py` (wired into
  `scripts/safety-sharpen-api-audit.sh`).
- Stored callbacks that receive raw frame/node pointers are declared with
  `unsafe (...)` function types.

### 1.2 Error types are per library and permanent

`FsError`, `NetError`, `TlsError`, `JsonParseError`, `RegexError`,
`CryptoError`, ... stay. `std::io::IoError` remains the shared I/O taxonomy,
with explicit conversions (`fs::io_error`, `fs::fs_error_from_io`, ...).

A future `std::error::Error` interface with `?` support is **additive**: it adds
impls and a conversion path, and does not change any existing signature. Do not
expect the per-library enums to be replaced.

### 1.3 `string_view` is the preferred text parameter type

- `string_view` accepts a string literal, a `std::string`, and another
  `string_view` (via the implicit `to_view` conversion).
- `&str` accepts a literal and a `string_view` (validating conversion), but
  **not** a `std::string` directly.
- `std::string` → `&str` and `&str` → `string_view` are not implicit.

Because most call sites hold a `std::string`, the low-friction parameter type is
`string_view`. Use `&str` only where UTF-8 validity must be load-bearing at the
signature. New APIs must not mix the two within one type; the known remaining
mix is tracked below.

### 1.4 Container accessor set

`vector`/`deque`/`span` expose a fixed accessor vocabulary. It is frozen:

| accessor | shape | notes |
|---|---|---|
| `get(i)` | `Option<&T>` | the default; never panics |
| `get_mut(i)` | `Option<&mut T>` | never panics |
| `get_ptr(i)` | `*mut T` | raw; dereferencing needs `unsafe` |
| `at(i)` | `T` | `T: Copy`; panics out of bounds |
| `at_ref(i)` | `&T` | panics out of bounds |
| `at_mut(i)` | `&mut T` | panics out of bounds |

Use `get`/`get_mut` in ordinary code; `at*` when the index is an invariant;
`get_ptr` only at a raw/FFI boundary.

### 1.5 Panic model

Panic aborts (no unwinding, no `catch_unwind`). Bounds failures, overflow,
and allocation failure abort through `__chc_*` trampolines; they are not
recoverable. Error reporting is `Result`, not `panic`.

`[no_panic]` is enforced per call (see §3.2 for its current reach).

---

## 2. What changed in this release (the one-time break)

- Public raw-pointer functions are `unsafe func` (migration table:
  `safety-sharpen/BREAKING.md`).
- Stored raw callbacks use `unsafe (...)` function types.
- `string`/`string_view`/`str` gained `as_bytes() : &[u8]` (additive, but the
  intended replacement for `unsafe(std::span<u8>(p, n))` helpers).
- `Option` gained `unwrap_or(&mut self, default : T) : T` (additive).

---

## 3. Known missing, and why each is additive

### 3.1 Unsized coercion `&[N]T` → `&[T]`

`f(data)` works for a fixed array, `f(&data)` does not, and `f(vec)` needs
`vec.as_span()`. Adding the coercion is a **front-end + lowering** feature
(reference-to-array becomes a `{ptr,len}` pair). It is additive: code written
today with `.as_span()`/by-value arrays keeps compiling.

Until then: pass the array by value, use `vec.as_span()`, or `s.as_bytes()`.

### 3.2 `[no_panic]` has no annotated callees yet

No library function is declared `[no_panic]`, so a `[no_panic]` body can call
almost nothing (`string("x")`, `vector()`, `println`, `fs::exists` are all
rejected). Making it usable requires deciding whether allocation failure and
bounds failure are *panics* (⇒ fallible `try_*` container APIs) or *traps*
(⇒ existing signatures can be annotated). Either direction is additive:
`vector::try_push`/`try_reserve` already exist, and adding `try_*` to `string`
does not invalidate the panicking `append`/`reserve`.

### 3.3 `Result`/`Option` combinators

`map`, `map_err`, `and_then`, `ok_or` need a method-level generic parameter
whose type is inferred from a callback. That inference does not work yet, and
explicit type arguments on a method are rejected, so these cannot be written
correctly today. They are additive when the inference lands.

Available now: `is_some`/`is_none`/`take`/`unwrap_or` (Option),
`is_ok`/`is_err` (Result).

### 3.4 No `?` operator

Error propagation is `switch` or pattern-else. Adding `?` later is additive.

### 3.5 `regex` mixes `string_view` and `&str`

`is_match`/`compile` take `string_view`; `find`/`captures`/`replace`/`split`
take `&str`. Standardizing needs either an implicit `std::string` → `&str`
conversion or a sweep to `string_view`; both are additive once decided
(§1.3 says `string_view` is the target).

### 3.6 Fallible `string` allocation

`string::append`/`reserve` panic on allocation failure; only `vector` has
`try_push`/`try_reserve`. Threading a failure flag through the SSO/heap
transitions is a contained refactor and is additive.

---

## 4. Defects found during readiness work

Fixed in this release:

- **Use-after-free through `Option<&T>`** (`SAFE-BORROW-001` now fires).
  `var r = v.get(0); v.push(..); *r` compiled and read freed memory. The
  borrow checker tracked a direct `&T` result (`at_ref`) but dropped the
  receiver loan when the reference was wrapped in a variant/aggregate. Fixed in
  `MIRBorrowCheck.cpp` (`returns_reference_aggregate`); negative tests:
  `neg_option_reference_borrow_survives_push`,
  `neg_option_reference_borrow_survives_clear`, and the NLL positive
  `option_reference_loan_ends_at_last_use`.

Open (documented, not silent):

- **A by-value `self` method on a variant segfaults the compiler.**
  ```chemical
  public variant Q<T> { Has(v : T)  Empty()
      func byvalue(self) : int { return 0 } }
  ```
  By-value `self` works on structs (`Pin::into_inner`, `SafeHasher::finish`,
  `http::Server::serve`), so this is variant-specific.
- **`[no_panic]` on `main` is ignored, and the entry symbol is not emitted**
  (`unresolved reference to 'main'` at link). Annotate a helper instead.
- A misspelled qualified name can resolve to an imported C function
  (`net::connect(...)` → cstd `connect`), producing a confusing parameter
  error instead of "unresolved". Prefer the real spelling
  (`net::TcpStream.connect(...)`).
- Documented codegen workarounds remain in library source: a `regex::Regex`
  temporary in an expression (heap corruption, per the regexp test-helper note)
  and `&raw local[0]` output buffers (`tls/raw.ch`).

---

## 5. Gates

| Gate | Command |
|---|---|
| Raw surface (`unsafe func`) | `scripts/safety-raw-surface-audit.py` |
| Public safe entry points reachable | `lang/tests/libs/safe_entrypoints_test.ch` (`--libs`) |
| Whole-tree unsafe growth | `scripts/safety-audit.sh` |
| Suites | `--tcc`, `--libs`, `--negative`, `--safety`, `--plugins`, `--interpret`, `--async`, `--server`, `--regexp` |

The entry-point gate exists because a "safe surface" can be written and never
reached: the audio/`image`/`font` helpers were namespace-level bare `func`
(internal). Every library's safe door must be referenced there.
