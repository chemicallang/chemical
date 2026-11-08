// `spawn_blocking` — the bridge from the blocking thread pool to the async
// runtime (design F4/F5, §7 Tier 0; integration doc §6).
//
// `spawn_blocking(f)` submits `f` to a lazily-created shared pool and returns a
// `FutureHandle<T>` that becomes `Ready` when the worker finishes. The task and
// its result both live in the shared state: the worker moves its result into a
// `std::Option<T>` slot and the consumer's poll `take`s it out. Nothing moves a
// `T` field out through a pointer (the B18 rule) and there is no separately
// allocated result buffer that could outlive its owner.
//
// The submitted closure captures only the state pointer (a small capture, stored
// inline by `std::function`), never the `std::function` itself; capturing the
// function would copy its heap-allocated capture and double-free it.
//
// Cancellation is safe: if the handle is dropped before the task completes, the
// worker observes `abandoned` and frees the shared state itself, so nothing is
// leaked and nothing is written after free.
public namespace async {

// Run `f` on the blocking pool and return its future. Await it with
// `await spawn_blocking(...)` / `async::block_on(...)`.
//
// The closure's captures are moved to a worker thread. `std::function` erases
// their types, so the compiler cannot prove `Send`/`'static`; this is an
// unsafe boundary until capture-type tracking lands (S6.5).
public unsafe func <T> spawn_blocking(f : std::function<() => T>) : core::async::FutureHandle<'static, T> {
        unsafe {
        var st = unsafe(malloc(sizeof(raw::BlockingState<T>))) as *mut raw::BlockingState<T>
        if(st == null) {
            panic("failed to allocate blocking state")
        }
        var vtbl = unsafe(malloc(sizeof(core::async::FutureTable<T>))) as *mut core::async::FutureTable<T>
        if(vtbl == null) {
            unsafe { dealloc st }
            panic("failed to allocate blocking vtable")
        }
        vtbl.poll = raw::blocking_poll<T>
        vtbl.drop = raw::blocking_drop<T>
        new(st) raw::BlockingState<T> {
            m : std::mutex(),
            ready : false,
            consumed : false,
            abandoned : false,
            value : std::Option.None<T>(),
            task : f,
            vtbl : vtbl
        }
        var pool = raw::blocking_pool()
        unsafe { pool.submit_void(|st|() => {
            var r = st.task()
            st.m.lock()
            var was_abandoned = st.abandoned
            if(!was_abandoned) {
                st.value = std::Option.Some<T>(r)
                st.ready = true
            }
            st.m.unlock()
            if(was_abandoned) {
                // `r` is destroyed by this scope; the state is ours to free
                unsafe {
                    delete st
                }
            }
        })
        }
        return unsafe(core::async::future_handle_raw<T>(st as *mut void, vtbl))

        }
    }

}
