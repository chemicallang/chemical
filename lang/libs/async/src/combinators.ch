// Future combinators: `select`, `timeout_or` and `block_on_timeout`
// (design F6/F7; Tier 0).
//
// All of these poll their child handle(s) on each executor pass, so they work
// with the polling executor in `exec.ch` without a timer wheel. A reactor/timer
// (Tier 1) can later register them instead of polling.
//
// The result type is the *same* as the child's (`T`), not a wrapper such as
// `Result<T, E>`: the vtable field type is then `FutureTable<T>` / `Poll<T>`
// with a bare generic parameter. A vtable written with a composite argument
// (`FutureTable<Result<T, E>>`) currently mis-resolves its member types inside a
// generic body (compiler blocker B20), so the combinators keep the payload type
// as a direct parameter.
public namespace async {

// Move the payload out of a `Ready` poll and leave `Pending` behind. Mirrors
// `std::Option::take`; generic runtime code needs it because moving a `T`
// payload bound by a variant pattern is rejected by the move checker (B18).
public func <T> poll_take_ready(p : *mut core::async::Poll<T>) : T {
    var temp : T
    unsafe {
        var Ready(value) = *p else unreachable
        memcpy(&raw mut temp, &raw value, sizeof(T))
        new(p) core::async::Poll.Pending<T>()
        return temp;
    }
}

// ---- select ----------------------------------------------------------------

public struct SelectState<T> {
    var a : core::async::FutureHandle<T>
    var b : core::async::FutureHandle<T>
    var vtbl : *mut core::async::FutureTable<T>
}

@retained
public func <T> select_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<T> {
    var st = frame as *mut SelectState<T>
    var pa = st.a.vtbl.poll(st.a.frame, cx)
    if(pa is core::async::Poll.Ready) {
        var va = poll_take_ready<T>(&raw mut pa)
        return core::async::Poll.Ready<T>(va)
    }
    var pb = st.b.vtbl.poll(st.b.frame, cx)
    if(pb is core::async::Poll.Ready) {
        var vb = poll_take_ready<T>(&raw mut pb)
        return core::async::Poll.Ready<T>(vb)
    }
    return core::async::Poll.Pending<T>()
}

@retained
public func <T> select_drop(frame : *mut void) {
    var st = frame as *mut SelectState<T>
    var vtbl = st.vtbl
    unsafe {
        delete st
    }
    if(vtbl != null) {
        unsafe {
            dealloc vtbl
        }
    }
}

// Become ready with whichever of `a`/`b` completes first. The other is
// cancelled when this future is dropped.
public func <T> select(a : core::async::FutureHandle<T>, b : core::async::FutureHandle<T>) : core::async::FutureHandle<T> {
    var st = malloc(sizeof(SelectState<T>)) as *mut SelectState<T>
    var vtbl = malloc(sizeof(core::async::FutureTable<T>)) as *mut core::async::FutureTable<T>
    vtbl.poll = select_poll<T>
    vtbl.drop = select_drop<T>
    new(st) SelectState<T> {
        a : a,
        b : b,
        vtbl : vtbl
    }
    return core::async::FutureHandle<T> { frame : st as *mut void, vtbl : vtbl }
}

// ---- timeout ---------------------------------------------------------------

public struct TimeoutOrState<T> {
    var inner : core::async::FutureHandle<T>
    var fallback : std::Option<T>
    var start_ms : i64
    var millis : i64
    var vtbl : *mut core::async::FutureTable<T>
}

@retained
public func <T> timeout_or_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<T> {
    var st = frame as *mut TimeoutOrState<T>
    var p = st.inner.vtbl.poll(st.inner.frame, cx)
    if(p is core::async::Poll.Ready) {
        var v = poll_take_ready<T>(&raw mut p)
        return core::async::Poll.Ready<T>(v)
    }
    if(std::now_milli() - st.start_ms >= st.millis) {
        var fb = st.fallback.take()
        return core::async::Poll.Ready<T>(fb)
    }
    return core::async::Poll.Pending<T>()
}

@retained
public func <T> timeout_or_drop(frame : *mut void) {
    var st = frame as *mut TimeoutOrState<T>
    var vtbl = st.vtbl
    unsafe {
        delete st
    }
    if(vtbl != null) {
        unsafe {
            dealloc vtbl
        }
    }
}

// Await `handle`, but if it has not completed within `millis` milliseconds
// cancel it and become `fallback` instead. Use a sentinel `fallback` to detect
// the timeout at the call site.
public func <T> timeout_or(handle : core::async::FutureHandle<T>, millis : u64, fallback : T) : core::async::FutureHandle<T> {
    var st = malloc(sizeof(TimeoutOrState<T>)) as *mut TimeoutOrState<T>
    var vtbl = malloc(sizeof(core::async::FutureTable<T>)) as *mut core::async::FutureTable<T>
    vtbl.poll = timeout_or_poll<T>
    vtbl.drop = timeout_or_drop<T>
    new(st) TimeoutOrState<T> {
        inner : handle,
        fallback : std::Option.Some<T>(fallback),
        start_ms : std::now_milli(),
        millis : millis as i64,
        vtbl : vtbl
    }
    return core::async::FutureHandle<T> { frame : st as *mut void, vtbl : vtbl }
}

// Synchronous variant: drive `handle` to completion on the current thread, but
// give up after `millis` and return `None`. The handle (and its frame) is
// dropped either way, cancelling an unfinished child.
public func <T> block_on_timeout(handle : core::async::FutureHandle<T>, millis : u64) : std::Option<T> {
    var e = executor()
    var cx = core::async::Context { waker : exec_make_waker(e) }
    var start = std::now_milli()
    var out : std::Option<T> = loop {
        var r = handle.vtbl.poll(handle.frame, &raw mut cx)
        if(r is core::async::Poll.Ready) {
            var v = poll_take_ready<T>(&raw mut r)
            break std::Option.Some<T>(v)
        }
        if(std::now_milli() - start >= millis as i64) {
            break std::Option.None<T>()
        }
        executor_poll_tasks(e, 4096u)
        executor_wait(e, 1u)
        continue
    }
    return out
}

}
