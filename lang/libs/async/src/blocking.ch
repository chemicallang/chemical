// `spawn_blocking` — the bridge from the blocking thread pool to the async
// runtime (design F4/F5, §7 Tier 0; integration doc §6).
//
// `spawn_blocking(f)` submits `f` to a lazily-created shared pool and returns a
// `FutureHandle<T>` that becomes `Ready` when the worker finishes. The worker
// bitwise-moves its result into heap storage and signals a mutex-guarded flag;
// the consumer's poll takes it out (never moving a `T` field through a pointer —
// the B18 rule). Cancellation is safe: if the handle is dropped before the task
// completes, the worker observes `abandoned` and frees the shared state itself,
// so nothing is leaked and nothing is written after free.
public namespace async {

public struct BlockingState<T> {
    var m : std::mutex
    var ready : bool
    var abandoned : bool
    var value : *mut T
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
    var p = st.value
    st.value = null
    st.ready = false
    st.m.unlock()
    var temp : T
    unsafe {
        memcpy(&raw mut temp, p, sizeof(T))
        free(p)
        return core::async::Poll.Ready<T>(temp)
    }
}

@retained
public func <T> blocking_drop(frame : *mut void) {
    var st = frame as *mut BlockingState<T>
    var vtbl = st.vtbl
    st.m.lock()
    if(st.ready) {
        // The result was never consumed: destroy it, then the state is ours.
        var p = st.value
        st.value = null
        st.ready = false
        st.m.unlock()
        if(p != null) {
            unsafe {
                delete p
                delete st
            }
        } else {
            unsafe {
                delete st
            }
        }
    } else {
        // The worker is still running and owns the state from here on.
        st.abandoned = true
        st.m.unlock()
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
        abandoned : false,
        value : null,
        vtbl : vtbl
    }
    var pool = blocking_pool()
    pool.submit_void(|st, f|() => {
        var r = f()
        st.m.lock()
        var was_abandoned = st.abandoned
        if(!was_abandoned) {
            var p = malloc(sizeof(T)) as *mut T
            memcpy(p, &raw r, sizeof(T))
            intrinsics::forget(r)
            st.value = p
            st.ready = true
        }
        st.m.unlock()
        if(was_abandoned) {
            delete st
        }
    })
    return core::async::FutureHandle<T> { frame : st as *mut void, vtbl : vtbl }
}

}
