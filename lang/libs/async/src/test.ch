// Deterministic test executor (design Sections 5.7 and 12.5).
//
// `test::block_on` drives a future to completion on the current thread without
// any reactor, parking the thread when the future reports `Pending`. It is the
// executor used by the `--libs` deterministic async tests.
public namespace async {
public namespace test {

public func <'a, T> block_on(handle : core::async::FutureHandle<'a, T>) : T {
    return async::block_on<T>(handle)
}

}
}
