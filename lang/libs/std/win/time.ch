@extern
protected struct FILETIME {
    var dwLowDateTime : u32
    var dwHighDateTime : u32
}

@extern
public func GetSystemTimeAsFileTime(lpSystemTimeAsFileTime : *mut FILETIME) : void

// QPC for high-resolution monotonic clock
@extern
public func QueryPerformanceCounter(lpPerformanceCount : *mut u64) : int

@extern
public func QueryPerformanceFrequency(lpFrequency : *mut u64) : int

public namespace std {

    public func now_milli() : i64 {
        var ft = zeroed<FILETIME>()
        GetSystemTimeAsFileTime(&mut ft)
        var time = (ft.dwHighDateTime as u64 << 32u64) | (ft.dwLowDateTime as u64)
        // 100-nanosecond intervals since Jan 1, 1601 to Jan 1, 1970
        // 116444736000000000 = number of 100-ns intervals from 1601 to 1970
        return ((time as i64 - 116444736000000000i64) / 10000i64)
    }

    internal func now_realtime(secs : *mut i64, nanos : *mut i64) {
        var ft = zeroed<FILETIME>()
        GetSystemTimeAsFileTime(&mut ft)
        var time = (ft.dwHighDateTime as u64 << 32u64) | (ft.dwLowDateTime as u64)
        // Convert 100-ns intervals since 1601 to secs/nanos since 1970
        var epoch_offset : i64 = 116444736000000000i64  // 100-ns intervals
        var total_100ns = time as i64 - epoch_offset
        *secs = total_100ns / 10000000i64
        *nanos = (total_100ns % 10000000i64) * 100i64
    }

    internal func now_monotonic(secs : *mut i64, nanos : *mut i64) {
        var counter : u64 = 0
        var freq : u64 = 0
        var qpc_result = QueryPerformanceCounter(&mut counter)
        if(qpc_result == 0) {
            std::panic("QueryPerformanceCounter failed")
        }
        var freq_result = QueryPerformanceFrequency(&mut freq)
        if(freq_result == 0 || freq == 0) {
            std::panic("QueryPerformanceFrequency failed")
        }
        // Convert to seconds and nanoseconds
        var total_secs = counter / freq
        var remainder = counter % freq
        // remainder / freq = fractional seconds
        // (remainder * 1e9) / freq = nanoseconds
        *secs = total_secs as i64
        *nanos = ((remainder * 1000000000u64) / freq) as i64
    }

}
