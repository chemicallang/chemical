// The single, explicit bridge from synchronous to asynchronous code
// (design D12). Blocking `block_on` is the bootstrap executor: it drives a
// future handle to completion on the current thread, then drops the handle
// (which cancels/frees the frame).
//
// On `Pending` the thread parks for a short interval before re-polling rather
// than busy-spinning. This makes clock-driven futures (`async::sleep`) and
// futures completed from another thread (`async::spawn_blocking`) work without
// requiring the future to own a waker. The full executor/reactor (design
// Section 12) replaces this with condvar parking + readiness wakeups.
public namespace async {

public func <T> block_on(handle : core::async::FutureHandle<T>) : T {
    var cx = core::async::Context {
        waker : core::async::Waker { data : null, vtbl : null }
    }
    var out : T = loop {
        var r = handle.vtbl.poll(handle.frame, &raw mut cx)
        if(r is core::async::Poll.Ready) {
            var Ready(value) = r else unreachable
            break value
        } else {
            std::concurrent.sleep_ms(1u)
            continue
        }
    }
    return out
}

}
