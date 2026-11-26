# No-panic / effect compatibility matrix

`[no_panic]` (and the Phase 3 effects `[no_alloc]`, `[no_unsafe]`,
`[no_blocking]`, `[pure]`) are enforced on the annotated function body. A
restriction is transitive: a call is accepted only when the callee carries the
same explicit restriction or is a declared effect-free extern. Generic
instantiations inherit the master declaration's attributes. Async functions keep
the explicit "not supported yet" error (see `NONBREAKING_DEFERRED.md`).

| Surface | Status | Notes |
|---|---|---|
| `lang/tests/safety/src/no_panic_sample.ch` | safe | Arithmetic, bounds and effect samples (`no_panic`, `no_alloc`, `no_unsafe`, `no_blocking`, `pure`, generic propagation); compiled and run by `--safety`. |
| `lang/tests/negative/src/effects.ch` | enforced | Positive/negative cases per effect, combined restrictions, conflicts, transitive callee rule, generic propagation, module defaults. |
| `std::Option` classification (`is_some`/`is_none`) | safe (`[no_panic]`) | Marked `[no_panic]`; the body is a variant tag test with no calls. |
| `std::Result::is_ok` / `is_err` | safe (`[no_panic]`) | Marked `[no_panic]`; same shape as `Option`. |
| `std::vector::try_reserve` / `try_push` / `try_ensure_capacity_for_one_more` | safe (fallible), not `[no_panic]` | Return `false` on allocation/capacity failure. Cannot be annotated yet: `try_reserve` calls the unannotated `malloc`/`realloc`, the div-zero guard is emitted for the (checked nonzero) element-size division, and `try_push` runs the generic `T` destructor. Recorded in `NONBREAKING_DEFERRED.md`. |
| `std::string` growth (`try_append*`, `try_reserve`) | safe (fallible) | `try_append`/`try_append_string`/`try_append_view`/`try_reserve` return `false` on capacity/allocation failure and leave the string unchanged. Panicking `append*`/`reserve` remain. Not yet `[no_panic]` for the same allocator/generic reason. |
| `std::Rc::try_new` / `std::try_rc`, `std::Arc::try_new` / `std::try_arc` | safe (fallible) | Return `Result<_, AllocError>`; the panicking `Rc::new`/`Arc::new` remain. |
| `std::deque` (`try_push`, `try_reserve_blocks`) | safe (fallible) | Return `false` on capacity/allocation failure; panicking `push` remains. |
| `std::unordered_map` / `ordered_map` (`try_insert`) | safe (fallible) | Return `false` on allocation/capacity failure; panicking `insert` remains. |
| `std::concurrent.ThreadPool::try_create_pool` | mixed | Reports the pool/queue allocation path as `Err`; platform thread creation still aborts. |
| `std::io` Read/Write boundaries | mixed | Result-returning; concrete implementations may still panic in legacy raw helpers. |
| `fs` safe path helpers | mixed | Return `Result`; legacy raw functions remain panicking/unsafe. |
| `net` safe TCP/TLS helpers | mixed | Return `Result`; allocation failures in vector growth are reported where `try_push` is used. |
| `process::execute`/`spawn` | mixed | Return `Result`; process setup still uses platform FFI. |
| async executor submission (`spawn*`) | mixed | Frame/vtable allocation still aborts on OOM; a fallible submission form is not shipped yet. |
| async function effect annotations | rejected (deferred) | `[no_*]`/`[pure]` on `async func` errors: the lowered coroutine ramp allocates a frame, and generated poll/drop/await machinery is not attributable to a source effect or explicitly annotated. See `NONBREAKING_DEFERRED.md`. |
| interface dynamic dispatch under a restriction | rejected (fail-closed) | `DynamicCall` has no provable effect set. Unannotated code is unaffected. |
| legacy raw APIs / cstd | panics | Tier C; not part of the safe API guarantee. |

Allocation-failure checks: `lang/tests/libs/std_safe_test.ch` exercises an
impossible `vector::try_reserve` capacity, an impossible `deque`
block capacity, an impossible `string` capacity, and the fallible
string/deque/map paths (must return `false`, not abort).
