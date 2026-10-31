// Async timers (design F6): `sleep` and friends.
//
// `sleep` returns a future that becomes `Ready(Unit)` once the wall clock has
// advanced past the requested duration. It is poll-driven (no background
// thread): `block_on` parks for a short interval and re-polls. A future
// executor will register a timer instead of polling.
public namespace async {

struct SleepState {
    var start_ms : i64
    var millis : i64
    var vtbl : *mut core::async::FutureTable<core::async::Unit>
}

func sleep_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<core::async::Unit> {
        unsafe {
        var st = frame as *mut SleepState
        if(std::now_milli() - st.start_ms >= st.millis) {
            return core::async::Poll.Ready<core::async::Unit>(core::async::Unit { })
        }
        return core::async::Poll.Pending<core::async::Unit>()

        }
    }

func sleep_drop(frame : *mut void) {
        unsafe {
        var st = frame as *mut SleepState
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
    }

public func sleep(millis : u64) : core::async::FutureHandle<'static, core::async::Unit> {
        unsafe {
        var st = unsafe(malloc(sizeof(SleepState))) as *mut SleepState
        if(st == null) {
            panic("failed to allocate sleep state")
        }
        st.start_ms = std::now_milli()
        st.millis = millis as i64
        var vtbl = unsafe(malloc(sizeof(core::async::FutureTable<core::async::Unit>))) as *mut core::async::FutureTable<core::async::Unit>
        if(vtbl == null) {
            unsafe { dealloc st }
            panic("failed to allocate sleep vtable")
        }
        vtbl.poll = sleep_poll
        vtbl.drop = sleep_drop
        st.vtbl = vtbl
        return unsafe(core::async::future_handle_raw<core::async::Unit>(st as *mut void, vtbl))

        }
    }

}
