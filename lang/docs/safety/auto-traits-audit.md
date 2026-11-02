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

## Explicit negative impls

| Type | Negative | Reason |
|---|---|---|
| `Rc<T>`, `Weak<T>` | `!Send`, `!Sync` | Counts are non-atomic; the raw-pointer representation is also negative, but the explicit impls keep a representation change from silently making them transferable. |
| `UnsafeCell<T>`, `Cell<T>`, `RefCell<T>` | `!Sync` | Interior mutability through `&self`; `Send` still follows `T`. |
| `Ref<'a,T>`, `RefMut<'a,T>` | `!Send`, `!Sync` | Guards carry a `&RefCell<T>` and release a non-atomic borrow state in their destructor. |
| `MutexGuard<T>`, `RwLockReadGuard<T>`, `RwLockWriteGuard<T>` | `!Send`, `!Sync` | Guards are tied to the locking thread; moving/dropping on another thread would unlock from the wrong thread. |
| `SendStaticCapture` | `!Sync` | Proves `Send` only; an owned capture may still contain interior mutability. |
| `ScopedSendCapture` | `!Send`, `!Sync` | May borrow caller storage; only a join-before-return scoped API may consume it. |
| `FnMutCapture`, `FnOnceCapture` | `!Send`, `!Sync` | Replayable/once callable markers have no thread witness. |
| Unbound `dyn I` | `!Send`, `!Sync`, `!Unpin` | The erased implementor may contain `Rc`/raw pointers. Explicit `+ Send`/`+ Sync` are construction-checked. |

## Deliberately conservative (false negatives, not unsoundness)

These are **not** made `Send`/`Sync` because their type does not carry the
lifetime witness that would make the transfer sound:

- `span<T>`, `mut_span<T>`, `str`, `string_view` store raw pointer/length
  pairs. A shared view should eventually be `Send`/`Sync` under `T: Sync`, but
  only after the view carries the borrow lifetime in the type; otherwise a
  view of a local could pass a `Send + 'static` boundary.
- `ThreadPool` and the thread-pool `Future<T>` hold raw pointers and are not
  part of the safe cross-thread surface.
- `std::function<F>` with an erased capture witness is conservatively
  `!Send`/`!Sync`; use `send_function`, `scoped_send_function`, or
  `typed_function` when a witness must be exposed.
- Conditional positive `Unpin` impls are still not evaluated in the nested
  path; this can reject, never accept an `!Unpin` value.

## Gates

The audit is covered by the negative safety suite and the existing
`arc`/`box`/`mutex`/`rwlock`/`thread`/`channel` tests, plus:

- TCC/LLVM main and negative suites
- TCC/LLVM safety and async suites
- TCC libs and plugins
- `MIRTests`
- `--llvm --thread-tsan`
- whole-tree safety audit (no growth)
