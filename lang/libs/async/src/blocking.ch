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

public struct BlockingState<T> {
    var m : std::mutex
    var ready : bool
    var consumed : bool
    var abandoned : bool
    var value : std::Option<T>
    var task : std::function<() => T>
    var vtbl : *mut core::async::FutureTable<T>
}

@retained
public func <T> blocking_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<T> {
    var st = frame as *mut BlockingState<T>
    st.m.lock()
    if(!st.ready) {
        st.m.unlock()
        return core::async::Poll.Pending<T>()
    }
    var taken = st.value.take()
    st.ready = false
    st.consumed = true
    st.m.unlock()
    return core::async::Poll.Ready<T>(taken)
}

@retained
public func <T> blocking_drop(frame : *mut void) {
    var st = frame as *mut BlockingState<T>
    var vtbl = st.vtbl
    var free_state = false
    st.m.lock()
    if(st.consumed) {
        // consumer already took the value; nothing left to destroy here
        free_state = true
    } else if(st.abandoned) {
        // worker is still running and will free the state itself
        free_state = false
    } else if(st.ready) {
        // worker finished but the result was never consumed; destroy it here
        st.ready = false
        free_state = true
    } else {
        // worker has not finished yet; hand ownership of the state to it
        st.abandoned = true
        free_state = false
    }
    st.m.unlock()
    if(free_state) {
        unsafe {
            delete st
        }
    }
    if(vtbl != null) {
        unsafe {
            dealloc vtbl
        }
    }
}

// Lazily created, process-wide blocking pool. `ThreadPool` owns native worker
// threads and frees them in its destructor; it is intentionally never torn down
// before process exit.
@never_destructed
var g_blocking_pool : std::concurrent.ThreadPool
var g_blocking_pool_ready : bool = false

@retained
public func blocking_pool() : *mut std::concurrent.ThreadPool {
    if(!g_blocking_pool_ready) {
        var n = std::concurrent.hardware_threads()
        if(n < 1u) {
            n = 1u
        }
        g_blocking_pool = std::concurrent.create_pool(n as uint)
        g_blocking_pool_ready = true
    }
    return &raw mut g_blocking_pool
}

// Run `f` on the blocking pool and return its future. Await it with
// `await spawn_blocking(...)` / `async::block_on(...)`.
public func <T> spawn_blocking(f : std::function<() => T>) : core::async::FutureHandle<T> {
    var st = malloc(sizeof(BlockingState<T>)) as *mut BlockingState<T>
    var vtbl = malloc(sizeof(core::async::FutureTable<T>)) as *mut core::async::FutureTable<T>
    vtbl.poll = blocking_poll<T>
    vtbl.drop = blocking_drop<T>
    new(st) BlockingState<T> {
        m : std::mutex(),
        ready : false,
        consumed : false,
        abandoned : false,
        value : std::Option.None<T>(),
        task : f,
        vtbl : vtbl
    }
    var pool = blocking_pool()
    pool.submit_void(|st|() => {
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
    return core::async::FutureHandle<T> { frame : st as *mut void, vtbl : vtbl }
}

}
