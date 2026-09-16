// `yield_now` — a future that yields once to the executor (design Section 5.7).
//
// It demonstrates the smallest possible Compiler-independent future: a frame
// holding a vtable pointer plus one bool. The first poll returns `Pending` and
// immediately re-wakes, the second returns `Ready(Unit)`. It is deterministic
// and needs no reactor, so it is a good smoke test for the runtime plumbing.
public namespace async {

struct YieldState {
    var yielded : bool
    var vtbl : *mut core::async::FutureTable<core::async::Unit>
}

func yield_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<core::async::Unit> {
    var st = frame as *mut YieldState
    if(st.yielded) {
        return core::async::Poll.Ready<core::async::Unit>(core::async::Unit { })
    }
    st.yielded = true
    if(cx.waker.vtbl != null) {
        cx.waker.wake()
    }
    return core::async::Poll.Pending<core::async::Unit>()
}

func yield_drop(frame : *mut void) {
    var st = frame as *mut YieldState
    var vtbl = st.vtbl
    unsafe {
        dealloc st
    }
    if(vtbl != null) {
        unsafe {
            dealloc vtbl
        }
    }
}

public func yield_now() : core::async::FutureHandle<core::async::Unit> {
    var st = malloc(sizeof(YieldState)) as *mut YieldState
    st.yielded = false
    var vtbl = malloc(sizeof(core::async::FutureTable<core::async::Unit>)) as *mut core::async::FutureTable<core::async::Unit>
    vtbl.poll = yield_poll
    vtbl.drop = yield_drop
    st.vtbl = vtbl
    return core::async::FutureHandle<core::async::Unit> { frame : st as *mut void, vtbl : vtbl }
}

}
