# Phase 2 no-panic compatibility matrix

`[no_panic]` is enforced on the annotated function body. Full generic/async/
interface effect propagation is P3.1, so this matrix records the Phase 2
sample and the known boundaries rather than claiming universal library support.

| Surface | Status | Notes |
|---|---|---|
| `lang/tests/safety/src/no_panic_sample.ch` | safe | Plain arithmetic/bounds sample; compiled by `--safety`. |
| `std::vector::try_reserve` / `try_push` | safe (fallible) | Return `false` on allocation/capacity failure; generic `[no_panic]` propagation is P3.1. |
| `std::string` growth (`try_append*`, `try_reserve`) | safe (fallible) | `try_append`/`try_append_string`/`try_append_view`/`try_reserve` return `false` on capacity/allocation failure and leave the string unchanged. Panicking `append*`/`reserve` remain. |
| `std::Rc::try_new` / `std::try_rc`, `std::Arc::try_new` / `std::try_arc` | safe (fallible) | Return `Result<_, AllocError>`; the panicking `Rc::new`/`Arc::new` remain. |
| `std::deque` (`try_push`, `try_reserve_blocks`) | safe (fallible) | Return `false` on capacity/allocation failure; panicking `push` remains. |
| `std::unordered_map` / `ordered_map` (`try_insert`) | safe (fallible) | Return `false` on allocation/capacity failure; panicking `insert` remains. |
| `std::concurrent.ThreadPool::try_create_pool` | mixed | Reports the pool/queue allocation path as `Err`; platform thread creation still aborts. |
| `std::Option` classification (`is_some`/`is_none`) | safe (runtime) | No panic. `take`/`take_or_none` keep the documented panic/forgery behavior. |
| `std::Result::is_ok` / `is_err` | safe (runtime) | No panic. |
| `std::io` Read/Write boundaries | mixed | Result-returning; concrete implementations may still panic in legacy raw helpers. |
| `fs` safe path helpers | mixed | Return `Result`; legacy raw functions remain panicking/unsafe. |
| `net` safe TCP/TLS helpers | mixed | Return `Result`; allocation failures in vector growth are reported where `try_push` is used. |
| `process::execute`/`spawn` | mixed | Return `Result`; process setup still uses platform FFI. |
| async executor submission (`spawn*`) | mixed | Frame/vtable allocation still aborts on OOM; a fallible submission form is not shipped yet. |
| legacy raw APIs / cstd | panics | Tier C; not part of the safe API guarantee. |
| async/generic/interface effects | untested | P3.1. |

Allocation-failure checks: `lang/tests/libs/std_safe_test.ch` exercises an
impossible `vector::try_reserve` capacity, an impossible `deque`
block capacity, an impossible `string` capacity, and the fallible
string/deque/map paths (must return `false`, not abort).
