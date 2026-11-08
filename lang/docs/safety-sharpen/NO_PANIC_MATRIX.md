# Phase 2 no-panic compatibility matrix

`[no_panic]` is enforced on the annotated function body. Full generic/async/
interface effect propagation is P3.1, so this matrix records the Phase 2
sample and the known boundaries rather than claiming universal library support.

| Surface | Status | Notes |
|---|---|---|
| `lang/tests/safety/src/no_panic_sample.ch` | safe | Plain arithmetic/bounds sample; compiled by `--safety`. |
| `std::vector::try_reserve` / `try_push` | safe (runtime) | Return `false` on allocation/capacity failure; generic `[no_panic]` propagation is P3.1. |
| `std::Option` classification (`is_some`/`is_none`) | safe (runtime) | No panic. `take`/`take_or_none` keep the documented panic/forgery behavior. |
| `std::Result::is_ok` / `is_err` | safe (runtime) | No panic. |
| `std::io` Read/Write boundaries | mixed | Result-returning; concrete implementations may still panic in legacy raw helpers. |
| `fs` safe path helpers | mixed | Return `Result`; legacy raw functions remain panicking/unsafe. |
| `net` safe TCP/TLS helpers | mixed | Return `Result`; allocation failures in vector growth are reported where `try_push` is used. |
| `process::execute`/`spawn` | mixed | Return `Result`; process setup still uses platform FFI. |
| `string` growth | panics | No fallible `try_*` string API in Phase 2; P3.2. |
| legacy raw APIs / cstd | panics | Tier C; not part of the safe API guarantee. |
| async/generic/interface effects | untested | P3.1. |

Allocation-failure checks: `lang/tests/libs/std_safe_test.ch` exercises an
impossible `vector::try_reserve` capacity (must return `false`, not abort).
