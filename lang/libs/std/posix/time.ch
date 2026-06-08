// clock_gettime is declared in condvar.ch

public namespace std {

    public func now_milli() : i64 {
        var ts = zeroed<timespec>();
        clock_gettime(0, &raw mut ts);
        return (ts.tv_sec as i64 * 1000i64) + (ts.tv_nsec as i64 / 1000000i64);
    }

    internal func clock_id_realtime() : int {
        // CLOCK_REALTIME is 0 on all POSIX systems
        return 0
    }

    internal func clock_id_monotonic() : int {
        // CLOCK_MONOTONIC = 1 on Linux, 6 on macOS/FreeBSD
        comptime if(def.linux) {
            return 1
        } else {
            return 6
        }
    }

    internal func now_clock(secs : *mut i64, nanos : *mut i64, clock_id : int) {
        var ts = zeroed<timespec>()
        var rc = clock_gettime(clock_id, &raw mut ts)
        if(rc != 0) {
            panic("clock_gettime failed")
        }
        *secs = ts.tv_sec as i64
        *nanos = ts.tv_nsec as i64
    }

    internal func now_monotonic(secs : *mut i64, nanos : *mut i64) {
        now_clock(secs, nanos, clock_id_monotonic())
    }

    internal func now_realtime(secs : *mut i64, nanos : *mut i64) {
        now_clock(secs, nanos, clock_id_realtime())
    }

}
