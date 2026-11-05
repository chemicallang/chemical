# Stdlib auto-trait audit

Scope: every explicit `Send`/`Sync`/`!Send`/`!Sync` implementation in
`lang/libs`, plus the structural derivation that backs the remaining types.
The goal is not Rust parity; it is that **no safe API can move or share a value
across threads unless the value's representation proves it is safe**.

## Audit result (2026-11-02)

The explicit impls below were reviewed against their invariants. The audit
found one compiler-side soundness bug in the shared-reference rule; it is fixed
and covered by regression tests.

### Bug found: `&T` was accepted as `Send` even when `T: !Sync`

`Send` for a shared reference must require `T: Sync`. The constraint checker
passed the queried interface as both the Send and Sync interface when it
recursed into a reference. A `T: Send` check therefore recursed into `&U` with
the **Send** interface where the **Sync** interface was needed. An explicit
`!Sync` (for example `Cell`/`RefCell`) was missed, and `&Cell<int>` /
`&RefCell<int>` was accepted as `Send`. Sending such a reference to a scoped
thread would permit concurrent reads/writes of the cell's non-atomic state.

Fixed in:

- `compiler/typeverify/TypeVerify.cpp` (`trait_constraint_satisfied` and
  `verify_where_clause`)
- `ast/structures/GenericImplDecl.cpp` (generic impl bound checking)

Regression tests:

- `neg_shared_reference_to_explicit_not_sync_is_not_send`
- `neg_shared_reference_to_ref_cell_is_not_send`
- `neg_generic_impl_bound_shared_ref_not_sync`
- `mutable_reference_to_send_not_sync_is_send` (positive: `&mut T` still needs
  `T: Send`, not `T: Sync`)

## Explicit positive impls

| Type | Impl | Invariant |
|---|---|---|
| `Arc<T>` | `Send + Sync` if `T: Send + Sync` | Counts are protected by one `ArcInner` mutex; the value can be shared and its last owner can drop on any thread. |
| `ArcWeak<T>` | `Send + Sync` if `T: Send + Sync` | Upgrade is test-and-increment under the same lock; it can produce an `Arc<T>`. |
| `Box<T>` | `Send` if `T: Send`; `Sync` if `T: Sync` | Owns one value; shared access only yields `&T`. |
| `vector<T>`, `deque<T>` | `Send` if `T: Send`; `Sync` if `T: Sync` | Owned storage; shared methods expose `&T`/raw pointers, mutation requires `&mut`. |
| `unordered_map<K,V>`, `ordered_map<K,V>` | `Send` if `K,V: Send`; `Sync` if `K,V: Sync` | Same ownership rule; shared `get_ptr` is a raw pointer. |
| `string`, `u16string` | `Send + Sync` unconditionally | Owned buffer with no interior mutability; mutation requires `&mut`. |
| `Mutex<T>` | `Send + Sync` if `T: Send` | `lock` hands out exclusive `&mut T`; the guard is thread-bound. |
| `RwLock<T>` | `Send` if `T: Send`; `Sync` if `T: Send + Sync` | Readers get `&T` (needs `Sync`), writers get `&mut T` (needs `Send`). |
| `JoinHandle<T>` | `Send + Sync` if `T: Send` | Joining returns `T` on the joining thread; handle methods require `&mut`. |
| `Waker` | `Send + Sync` unconditionally | The vtable contract requires wake/clone/drop to be callable from any thread; `from_raw` is unsafe, and `empty()` is a null no-op. |
| `SendFutureHandle<T>` | `Send + Sync` if `T: Send` | A frame is promoted here only after `spawn_send` proves `Send + 'static`; construction is unsafe/private and `Sync` only exposes raw pointer reads. |
| `Sender<T>`, `Receiver<T>`, `Channel<T>` | `Send + Sync` if `T: Send` | All counters, queue links, and wakeups are under `ChannelShared.m`; handles cannot be forged. |
| `std::concurrent.Future<T>` | `Send` if `T: Send` | Owns the consumer side of one promise; the promise state is mutex/condvar protected and `get` is the only operation that transfers `T`. |
| `std::concurrent.ThreadPool` | `Send + Sync` | The private `PoolData` is heap-allocated; safe submission locks its mutex and the safe closure entry point requires a `Send + 'static` capture witness. Unchecked `submit`/`submit_void` stay unsafe. |

## Explicit negative impls

| Type | Negative | Reason |
|---|---|---|
| `Rc<T>`, `Weak<T>` | `!Send`, `!Sync` | Counts are non-atomic; the raw-pointer representation is also negative, but the explicit impls keep a representation change from silently making them transferable. |
| `UnsafeCell<T>`, `Cell<T>`, `RefCell<T>` | `!Sync` | Interior mutability through `&self`; `Send` still follows `T`. |
| `Ref<'a,T>`, `RefMut<'a,T>` | `!Send`, `!Sync` | Guards carry a `&RefCell<T>` and release a non-atomic borrow state in their destructor. |
| `MutexGuard<T>`, `RwLockReadGuard<T>`, `RwLockWriteGuard<T>` | `!Send`, `!Sync` | Guards are tied to the locking thread; moving/dropping on another thread would unlock from the wrong thread. |
| `lock_guard`, `recursive_lock_guard` | `!Send`, `!Sync` | Legacy RAII lockers hold a raw mutex pointer and unlock in their destructor; they must stay on the locking thread. |
| `SendStaticCapture` | `!Sync` | Proves `Send` only; an owned capture may still contain interior mutability. |
| `ScopedSendCapture` | `!Send`, `!Sync` | May borrow caller storage; only a join-before-return scoped API may consume it. |
| `FnMutCapture`, `FnOnceCapture` | `!Send`, `!Sync` | Replayable/once callable markers have no thread witness. |
| Unbound `dyn I` | `!Send`, `!Sync`, `!Unpin` | The erased implementor may contain `Rc`/raw pointers. Explicit `+ Send`/`+ Sync` are construction-checked. |

## Async, executor, and iterator coverage

Public async value types are checked directly:

| Type | Auto trait | Reason |
|---|---|---|
| `Poll<T>` | `Send`/`Sync` follows `T` | Variant payload only. |
| `Waker` | `Send + Sync` | Vtable contract requires cross-thread wake/clone/drop; `from_raw` is unsafe. |
| `Context` | `Send + Sync` when its waker is | Carries only the waker. |
| `SendFutureHandle<T>` | `Send + Sync` when `T: Send` | Frame is promoted only after `spawn_send` proves `Send + 'static`; construction is private/unsafe. |
| `FutureHandle<'a,T>` | conservatively `!Send`/`!Sync` | The erased frame may borrow the caller; crossing threads uses `SendFutureHandle`. |

Executor/future state structures (`Executor`, `RawTask`, `TaskVTable`,
`JoinState<T>`, `SpawnTask<T>`, `SelectState<T>`, `TimeoutOrState<T>`, and
`BlockingState<T>`) hold raw pointers or erased callbacks and carry explicit
`!Send`/`!Sync` impls, so a representation refactor cannot silently make them
transferable. The iterator `Chunk<T>` remains structurally `!Send`/`!Sync` via
its raw pointer/length fields. These are internal runtime state, not values
exchanged through the safe cross-thread API; turning any of them into a safe
transferable value requires a dedicated witness/API, not a blanket auto-trait
impl. The executor's public raw-state entry points
(`exec_make_waker`, `executor_set_wake_hook`, `exec_enqueue_task`,
`executor_poll_tasks`, `executor_wait`) are explicit `unsafe func`s and reject
null state; queued entries with a null frame/vtable are skipped.

## Nested `Unpin`

`Unpin` now uses the same explicit-impl context as `Send`/`Sync`, so a
conditional positive impl is evaluated at a concrete nested instantiation:

```chemical
struct Wrapper<T> {
    var marker : core::PhantomPinned
    var value : T
}

impl<T> core::Unpin for Wrapper<T> where T: core::Unpin { }

struct Outer { var w : Wrapper<int> }
```

`Outer` is `Unpin` for `Wrapper<int>` (the positive impl applies), and remains
`!Unpin` for `Wrapper<PhantomPinned>` (the bound fails). A conditional
`!Unpin` impl is still checked first and wins. The generic-impl bound path in
`GenericImplDecl.cpp` now retries after materializing concrete conditional
positive `Unpin` impls found in nested array/generic/field types; arguments that
remain generic parameters stay conservative. This is a false-rejection fix, not
an unsound acceptance: `Wrapper<PhantomPinned>` still fails its `T: Unpin`
bound.

## Deliberately conservative (false negatives, not unsoundness)

These are **not** made `Send`/`Sync` because their type does not carry the
lifetime witness that would make the transfer sound:

- `span<T>`, `mut_span<T>`, `str`, `string_view` store raw pointer/length
  pairs. A shared view should eventually be `Send`/`Sync` under `T: Sync`, but
  only after the view carries the borrow lifetime in the type; otherwise a
  view of a local could pass a `Send + 'static` boundary.
- `std::function<F>` with an erased capture witness is conservatively
  `!Send`/`!Sync`; use `send_function`, `scoped_send_function`, or
  `typed_function` when a witness must be exposed.
- Conditional positive `Unpin` impls are evaluated in the type-checker and
  dynamic-object paths; the generic-impl bound path still conservatively
  rejects them because it has no type-checker context.

## Gates

The audit is covered by the negative safety suite and the existing
`arc`/`box`/`mutex`/`rwlock`/`thread`/`channel` tests, plus:

- TCC main `2489/2489`, LLVM main `2487/2487`
- TCC/LLVM negative `850/850`
- TCC/LLVM safety `88/88`, async `83/83`
- TCC libs `736/736`, plugins `1305/1305`
- TCC interpret `1938/1938`
- `MIRTests` `4749 checks`
- `--llvm --thread-tsan` pass (`Mutex`, `RwLock`, `channel`, thread pool, executor spawn/yield`)
- whole-tree safety audit: no growth
