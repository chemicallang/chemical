// Re-export the core async protocol so users only need `import async`
// (design Section 5.6/5.7). `block_on` lives in block_on.ch.
public namespace async {

    using core::async::Poll;
    using core::async::WakerVTable;
    using core::async::Waker;
    using core::async::Context;
    using core::async::Future;
    using core::async::FutureTable;
    using core::async::FutureHandle;
    using core::async::Unit;

}
