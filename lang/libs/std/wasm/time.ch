// Freestanding wasm time.
//
// There is no OS clock. The JS host provides `env.chc_now_millis` (wall-clock
// milliseconds, the same clock `Date.now()` uses). An unhosted run that never
// reaches a clock call is unaffected; a run that does must supply the import.
// Monotonic is the same clock in this wave (documented limitation).

@extern
public func chc_now_millis() : double

public namespace std {

    public func now_milli() : i64 {
        return unsafe(chc_now_millis()) as i64
    }

    internal func now_clock(secs : *mut i64, nanos : *mut i64, clock_id : int) { unsafe {
        const ms = chc_now_millis()
        const whole = ms as i64
        *secs = whole / 1000i64
        *nanos = (whole % 1000i64) * 1000000i64
    }}

    internal func now_monotonic(secs : *mut i64, nanos : *mut i64) {
        now_clock(secs, nanos, 0)
    }

    internal func now_realtime(secs : *mut i64, nanos : *mut i64) {
        now_clock(secs, nanos, 0)
    }

}
