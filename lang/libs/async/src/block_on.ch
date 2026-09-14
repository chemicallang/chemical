// The single, explicit bridge from synchronous to asynchronous code
// (design D12). Blocking `block_on` is the bootstrap executor: it drives a
// future handle to completion on the current thread, then drops the handle
// (which cancels/frees the frame).
public namespace async {

// Poll the handle until it is Ready, then move the result out. The handle is
// consumed by value, so its @delete runs on every return path (completion or
// early unwind), cancelling a suspended future exactly once.
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
            continue
        }
    }
    return out
}

}
